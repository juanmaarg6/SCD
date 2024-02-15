// Nombre: Juan Manuel Rodríguez Gómez
// DNI: 49559494Z
// Grupo: SCD 3, Doble Grado Ingeniería Informática y Matemáticas

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_productores = 4,  // Número de productores totales
   num_consumidores = 5; // Número de consumidores totales

const int
   num_procesos_esperado = num_productores + num_consumidores + 1;   // Número de procesos esperados (num_productores + num_consumidores + buffer)

const int
   id_buffer = num_productores,  // Identificador del buffer
   tam_vector = 10;              // Tamaño del vector del buffer

const int
   multiplo = 2,
   num_items = multiplo * num_productores * num_consumidores,  // Número de ítems a producir/consumir
   items_prod = num_items / num_productores,                   // Número de ítems que produce cada productor
   items_cons = num_items / num_consumidores;                  // Número de ítems que consume cada consumidor

const int
   etiq_productor = 1,
   etiq_consumidor = 2;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
   static default_random_engine generador( (random_device())() );
   static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
   return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------
// produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatoria

int producir(int num_productor)
{
   static int valor = num_productor * items_prod - 1;

   // Espera bloqueada
   sleep_for( milliseconds( aleatorio<10,100>()) );

   valor++ ;

   cout << "Productor " << num_productor << " ha producido el valor " << valor << endl << flush;

   return valor ;
}

// ---------------------------------------------------------------------

void funcion_productor(int num_productor)
{
   for ( unsigned int i = 0 ; i < items_prod ; i++ )
   {
      // Producir un valor
      int valor_prod = producir(num_productor);

      // Enviar un valor al buffer
      cout << "Productor " << num_productor << " va a enviar el valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_productor, MPI_COMM_WORLD );
   }
}

// ---------------------------------------------------------------------

void consumir(int num_consumidor, int valor_cons)
{
   // Espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );

   cout << "Consumidor " << num_consumidor << " ha consumido el valor " << valor_cons << endl << flush ;
}

// ---------------------------------------------------------------------

void funcion_consumidor(int num_consumidor)
{
   int peticion,       // Petición para consumir un valor del buffer
       valor_rec = 1;  // Valor recibido

   MPI_Status estado;  // Metadatos del mensaje recibido

   for( unsigned int i = 0 ; i < items_cons; i++ )
   {  
      // Envío de petición para consumir un valor del buffer
      MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);

      // Recepción valor del buffer
      MPI_Recv (&valor_rec, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD, &estado);
      cout << "Consumidor " << num_consumidor << " ha recibido valor " << valor_rec << endl << flush ;

      consumir(num_consumidor, valor_rec);
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int buffer[tam_vector],      // Buffer con celdas ocupadas y vacías
       valor,                   // Valor recibido o enviado
       primera_libre       = 0, // Índice de primera celda libre
       primera_ocupada     = 0, // Índice de primera celda ocupada
       num_celdas_ocupadas = 0, // Número de celdas ocupadas
       etiq_aceptada ;          // Etiqueta aceptada del mensaje
   
   MPI_Status estado ;          // Metadatos del mensaje recibido

   for( unsigned int i = 0 ; i < num_items*2 ; i++ )
   {
      // Determinar si puede enviar solo prod., solo cons, o todos

      // Si el buffer está vacío, solo se admiten mensajes de productores
      if(num_celdas_ocupadas == 0)
         etiq_aceptada = etiq_productor ;
      // Si el buffer está lleno, solo se admiten mensajes de consumidores
      else if (num_celdas_ocupadas == tam_vector)
         etiq_aceptada = etiq_consumidor ;
      // Si el buffer no está vacío ni lleno completamente, 
      // se admiten mensajes tantos de productores como de consumidors
      else                 
         etiq_aceptada = MPI_ANY_SOURCE ; 

      // El buffer recibe el mensaje con la etiqueta aceptada correspondiente
      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptada, MPI_COMM_WORLD, &estado );

      // Procesamos el mensaje recibido

      // Si el mensaje recibido es del productor, se inserta el valor producido en el buffer
      if(estado.MPI_TAG == etiq_productor) {
         buffer[primera_libre] = valor ;
         primera_libre = (primera_libre+1) % tam_vector ;
         num_celdas_ocupadas++ ;
         cout << "Buffer ha recibido valor " << valor << endl ;
      }

      // Si el mensaje recibido es del consumidor, se extrae el del buffer y se envía a un consumidor
      if(estado.MPI_TAG == etiq_consumidor) {
         valor = buffer[primera_ocupada] ;
         primera_ocupada = (primera_ocupada+1) % tam_vector ;
         num_celdas_ocupadas-- ;
         cout << "Buffer va a enviar valor " << valor << endl ;
         MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_consumidor, MPI_COMM_WORLD);
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // Inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // Ejecutar la función correspondiente a 'id_propio'
      if ( id_propio < id_buffer)         // Si es menor que el id del buffer
         funcion_productor(id_propio);    //   es un productor
      else if ( id_propio == id_buffer )  // Si coincide con el id del buffer
         funcion_buffer();                //   es el buffer
      else                                // Si es mayor que el id del buffer
         funcion_consumidor(id_propio);   //   es un consumidor
   }
   else
   {
      if ( id_propio == 0 ) // Solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // Al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
