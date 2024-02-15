/**********************************************************************/
// Nombre: Juan Manuel
// Apellidos: Rodríguez Gómez
// DNI: 49559494Z
// Curso: 3º Doble Grado en Ingeniería Informática y Matemáticas
// Grupo de Prácticas: SCD3
/**********************************************************************/

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: multprodcons2_su.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con múltiples productores y múltiples consumidores.
// Opcion FIFO
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------

/**
* =====================================================================
* MODIFICACIONES REALIZADAS PARA MÚLTIPLOS PROD/CONS
*
* Variables compartidas:
*   -> Crear num_productores y num_consumidores
*
* producir_dato(int num_hebra):
*   -> Se pone como parámetro num_hebra para identificar la hebra que la invoca
*   -> Se produce uniformemente cada dato (en orden)
*
* consumir_dato(unsigned dato, int num_hebra):
*   -> Se pone como parámetro num_hebra para identificar la hebra que la invoca
*
* funcion_hebra_productora(int num_hebra):
*   -> Bucle hasta datos_producidos_hebra
*   -> Se llama a la nueva funcion
*
* funcion_hebra_consumidora(int num_hebra):
*   -> Bucle hasta datos_consumidos_hebra
*   -> Se llama a la nueva funcion
*
* Main:
*   -> Assert para comprobar que los num_productores y num_consumidores son correctos
*   -> Vector de hebras
*
* =====================================================================
*/

#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

constexpr int
   num_items  = 40,      // número de items a producir/consumir
   num_productores = 10, // número de productores totales
   num_consumidores = 5, // número de consumidores totales
   datos_producidos_hebra = num_items / num_productores,
   datos_consumidos_hebra = num_items / num_consumidores;

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items],                           // contadores de verificación: producidos
   cont_cons[num_items],                           // contadores de verificación: consumidos
   producciones_hebras[num_productores] = {0},     // Cuenta las veces producidas por cada hebra
   consumiciones_hebras[num_consumidores] = {0};   // Cuenta las veces consumidas por cada hebra

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int num_hebra)
{
   int dato = num_hebra * datos_producidos_hebra + producciones_hebras[num_hebra];
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   mtx.lock();
   cout << "[Hebra: " << num_hebra << "] producido: " << dato << endl << flush ;
   mtx.unlock();

   producciones_hebras[num_hebra]++;
   cont_prod[dato]++;
   return dato;
}
//----------------------------------------------------------------------

void consumir_dato(unsigned dato, int num_hebra)
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }

   cont_cons[dato]++;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  [Hebra: " << num_hebra << "] consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version FIFO, semántica SU, múltiples prod. y múltiples cons.

class MultProdCons2SU : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre,           //  indice de celda de la próxima inserción
   primera_ocupada,         //  indice de celda de la próxima inserción
   num_celdas_ocupadas;     //  comprueba el número de celdas ocupadas en el buffer
 CondVar         // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   MultProdCons2SU(  ) ;    // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

MultProdCons2SU::MultProdCons2SU(  )
{
   primera_libre = 0 ;
   primera_ocupada = 0;
   num_celdas_ocupadas = 0;

   ocupadas = newCondVar();
   libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int MultProdCons2SU::leer(  )
{
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( num_celdas_ocupadas == 0 )
      ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < num_celdas_ocupadas );
   const int valor = buffer[primera_ocupada] ;

   primera_ocupada = (primera_ocupada + 1) % num_celdas_total;
   num_celdas_ocupadas--;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void MultProdCons2SU::escribir( int valor )
{
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( num_celdas_ocupadas == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( num_celdas_ocupadas < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre = (primera_libre + 1) % num_celdas_total;
   num_celdas_ocupadas++;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<MultProdCons2SU> monitor, int num_hebra )
{
   for( unsigned i = 0 ; i < datos_producidos_hebra ; i++ )
   {
      int valor = producir_dato(num_hebra) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<MultProdCons2SU> monitor, int num_hebra )
{
   for( unsigned i = 0 ; i < datos_consumidos_hebra ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor, num_hebra ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   // Comprobación de parámetros
   assert((num_items % num_productores == 0) && (num_items % num_consumidores == 0));

   cout << "----------------------------------------------------------------------------------"      << endl
        << "Problema de los productores-consumidores (Mult prod/cons, Monitor SU, buffer FIFO). "    << endl
        << "----------------------------------------------------------------------------------"      << endl
        << flush ;

   MRef<MultProdCons2SU> monitor = Create<MultProdCons2SU>();

   thread hebras_productoras[num_productores];
   thread hebras_consumidoras[num_consumidores];

   for(int i = 0; i < num_productores; i++)
      hebras_productoras[i] = thread(funcion_hebra_productora, monitor, i);

   for(int i = 0; i < num_consumidores; i++)
      hebras_consumidoras[i] = thread(funcion_hebra_consumidora, monitor, i);

   for(int i = 0; i < num_productores; i++)
      hebras_productoras[i].join();

   for(int i = 0; i < num_consumidores; i++)
         hebras_consumidoras[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
