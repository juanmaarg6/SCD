#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer
unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)

Semaphore 
   libres(tam_vec),  // Semáforo que cuenta las posiciones libres del buffer
   ocupadas(0),      // Semáforo que cuenta las posiciones ocupadas del buffer
   gestion_buffer(1);
unsigned int 
   buffer[tam_vec] = {0},  // Buffer para insertar datos
   primera_libre = 0;      // Índice en el buffer de la primera celda libre
//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato()
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = siguiente_dato ;
   siguiente_dato++ ;
   cont_prod[dato_producido] ++ ;
   cout << "producido: " << dato_producido << endl << flush ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato = producir_dato();

      // INICIO SECCIÓN CRÍTICA

      libres.sem_wait();
      gestion_buffer.sem_wait();

      // Inserción del dato
      assert(0 <= primera_libre && primera_libre < tam_vec);
      buffer[primera_libre++] = dato;
      cout << "insertado de buffer: " << buffer[primera_libre-1] << endl;

      gestion_buffer.sem_signal();
      ocupadas.sem_signal();

      // FIN SECCIÓN CRÍTICA
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato;

      // INICIO SECCIÓN CRÍTICA
      
      ocupadas.sem_wait();
      gestion_buffer.sem_wait();

      // Extracción del dato
      assert(0 <= primera_libre && primera_libre < tam_vec);
      dato = buffer[--primera_libre];
      cout << "extraido de buffer: " << dato << endl;

      libres.sem_signal();
      gestion_buffer.sem_signal();

      // FIN SECCIÓN CRÍTICA

      consumir_dato(dato) ;
   }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO o FIFO ?)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora ( funcion_hebra_productora ),
          hebra_consumidora( funcion_hebra_consumidora );

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores();
}
