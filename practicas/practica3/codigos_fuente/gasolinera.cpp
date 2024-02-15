// Nombre: Juan Manuel Rodríguez Gómez
// DNI: 49559494Z
// Grupo: SCD 3, Doble Grado Ingeniería Informática y Matemáticas

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: gasolinera.cpp
// Implementación del problema de la gasolinera.
//
// -----------------------------------------------------------------------------

#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_coches = 10,                     // Número de coches 
   num_procesos  = num_coches + 1 ;     // Número de procesos total (hay coches y la gasolinera)

const int 
   id_gasolinera = num_procesos - 1;    // Identificador de la gasolinera

const int 
   etiq_comenzar_repostar = 1,      // Etiqueta para mensajes de comenzar a repostar
   etiq_terminar_repostar = 2;      // Etiqueta para mensajes de terminar de repostar

const int 
   max_num_surtidores_en_uso = 5;   // Máximo número de surtidores que se pueden estar usando a la vez en la gasolinera


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

void funcion_coche( int id )
{

  while ( true )
  {   
      // Comenzar a repostar
      cout <<"Coche " <<id << " solicita comenzar a repostar" <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_comenzar_repostar, MPI_COMM_WORLD);

      // Repostando
      cout <<"Coche " <<id <<" esta repostando" <<endl ;
      sleep_for( milliseconds( aleatorio<10,100>() ) );

      // Terminar de repostar
      cout <<"Coche " <<id << " solicita terminar de repostar" <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_terminar_repostar, MPI_COMM_WORLD);

      // Espera para solicitar comenzar a repostar
      cout << "Coche " << id << " espera para solicitar comenzar a repostar" << endl;
      sleep_for( milliseconds( aleatorio<10,100>() ) );
   }
}

// ---------------------------------------------------------------------

void funcion_gasolinera()
{
   int valor,                    // Valor recibido
       etiq_aceptada,            // Etiqueta aceptada del mensaje
       num_surtidores_en_uso;   // Número de surtidores de la gasolinera que están siendo usados
   
   MPI_Status estado;   // Metadatos de las dos recepciones

   while ( true ) 
   {
      // Si en la gasolinera ya se ha alcanzado el número máximo de surtidores que pueden estar en uso a la vez,
      // solo se admiten mensajes de terminar de repostar
      if(num_surtidores_en_uso == max_num_surtidores_en_uso)
         etiq_aceptada = etiq_terminar_repostar;
      // Si en la gasolinera hay surtidores en uso pero todavía se puede usar algún surtidor más porque está libre,
      // se admiten mensajes tanto de comenzar a repostar como de terminar de repostar
      else
         etiq_aceptada = MPI_ANY_TAG;

      // No añadimos la opción if(num_surtidores_en_uso == 0) then etiq_aceptada = etiq_comenzar_repostar;
      // ya que al no haber surtidores en uso nunca se va a recibir un mensaje para terminar de repostar

      // El camarero recibe el mensaje con la etiqueta aceptada correspondiente
      MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptada, MPI_COMM_WORLD, &estado);

      // Procesamos el mensaje recibido

      // Gestionamos el número de surtidores de la gasolinera que están siendo usados en cada momento
      // Mostramos por pantalla la aceptación de la gasolinera de la petición de comenzar (o de terminar) de repostar en un surtidor por parte de un coche
      if(estado.MPI_TAG == etiq_comenzar_repostar) {
         num_surtidores_en_uso++;
         cout << "Gasolinera acepta la peticion de coche " << estado.MPI_SOURCE << " de comenzar a repostar" << endl; 
      }
      
      if(estado.MPI_TAG == etiq_terminar_repostar) {
         num_surtidores_en_uso--;
         cout << "Gasolinera acepta la peticion de coche " << estado.MPI_SOURCE << " de terminar de repostar" << endl; 
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   // Inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // Ejecutar la función correspondiente a 'id_propio'
      if ( id_propio == id_gasolinera )     // Si coincide con el id de la gasolinera
         funcion_gasolinera();              //   es la gasolinera
      else                                  // Si no               
         funcion_coche( id_propio );        //   es un coche
   }
   else
   {
      if ( id_propio == 0 ) // Solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // Al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------