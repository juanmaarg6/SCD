// Nombre: Juan Manuel Rodríguez Gómez
// DNI: 49559494Z
// Grupo: SCD 3, Doble Grado Ingeniería Informática y Matemáticas

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: gasolinera-2.cpp
// Extensión del problema 1 de la gasolinera. Ahora la gasolinera sirve tres tipos de combustible en 
// tres tipos de surtidores, y cada proceso coche necesita un tipo de combustible distinto
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
   etiq_comenzar_repostar_combust_0 = 1,      // Etiqueta para mensajes de comenzar a repostar combustible del tipo 0
   etiq_comenzar_repostar_combust_1 = 2,      // Etiqueta para mensajes de comenzar a repostar combustible del tipo 1
   etiq_comenzar_repostar_combust_2 = 3,      // Etiqueta para mensajes de comenzar a repostar combustible del tipo 2
   etiq_terminar_repostar_combust_0 = 4,      // Etiqueta para mensajes de terminar de repostar combustible del tipo 0
   etiq_terminar_repostar_combust_1 = 5,      // Etiqueta para mensajes de terminar de repostar combustible del tipo 1
   etiq_terminar_repostar_combust_2 = 6;      // Etiqueta para mensajes de terminar de repostar combustible del tipo 2



const int
   surtidores[3] = {2, 3, 4};       // Número de surtidores para cada tipo de combustible 
                                    // (la suma de todos los surtidores debe ser menor que el número de coches)


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

void funcion_coche( int id, int tipo_combustible )
{

  while ( true )
  {   
      // Comenzar a repostar
      cout <<"Coche " <<id << " solicita comenzar a repostar combustible tipo " << tipo_combustible <<endl;
      if(tipo_combustible == 0)
        MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_comenzar_repostar_combust_0, MPI_COMM_WORLD);
      else if(tipo_combustible == 1)
        MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_comenzar_repostar_combust_1, MPI_COMM_WORLD);
      else
        MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_comenzar_repostar_combust_2, MPI_COMM_WORLD);
      
      // Repostando
      cout <<"Coche " <<id <<" esta repostando combustible tipo " << tipo_combustible <<endl ;
      sleep_for( milliseconds( aleatorio<10,100>() ) );

      // Terminar de repostar
      cout <<"Coche " <<id << " solicita terminar de repostar combustible tipo " << tipo_combustible <<endl;
      if(tipo_combustible == 0)
        MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_terminar_repostar_combust_0, MPI_COMM_WORLD);
      else if(tipo_combustible == 1)
        MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_terminar_repostar_combust_1, MPI_COMM_WORLD);
      else
        MPI_Ssend( &id,  1, MPI_INT, id_gasolinera, etiq_terminar_repostar_combust_2, MPI_COMM_WORLD);

      // Espera para solicitar comenzar a repostar
      cout << "Coche " << id << " espera para solicitar comenzar a repostar combustible tipo " << tipo_combustible << endl;
      sleep_for( milliseconds( aleatorio<10,100>() ) );
   }
}

// ---------------------------------------------------------------------

void funcion_gasolinera()
{
   int valor,                    // Valor recibido
       etiq_aceptada,            // Etiqueta aceptada del mensaje
       surtidores_tipo_0_en_uso = 0, 
       surtidores_tipo_1_en_uso = 0, 
       surtidores_tipo_2_en_uso = 0,
       flag;
   
   MPI_Status estado;   // Metadatos de las dos recepciones

   while ( true ) 
   {

      MPI_Iprobe(MPI_ANY_SOURCE, etiq_terminar_repostar_combust_0, MPI_COMM_WORLD, &flag, &estado);
      if(flag > 0) {
         MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_terminar_repostar_combust_0, MPI_COMM_WORLD, &estado);
         surtidores_tipo_0_en_uso--;
      }
      else {
         MPI_Iprobe(MPI_ANY_SOURCE, etiq_terminar_repostar_combust_1, MPI_COMM_WORLD, &flag, &estado);
         if(flag > 0) {
            MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_terminar_repostar_combust_1, MPI_COMM_WORLD, &estado);
            surtidores_tipo_1_en_uso--;
         }
         else {
            MPI_Iprobe(MPI_ANY_SOURCE, etiq_terminar_repostar_combust_2, MPI_COMM_WORLD, &flag, &estado);
            if(flag > 0) {
               MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_terminar_repostar_combust_2, MPI_COMM_WORLD, &estado);
               surtidores_tipo_2_en_uso--;
            }
            else {
               MPI_Iprobe(MPI_ANY_SOURCE, etiq_comenzar_repostar_combust_0, MPI_COMM_WORLD, &flag, &estado);
               if(flag > 0) {
                  if(surtidores_tipo_0_en_uso < surtidores[0]) {
                     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_comenzar_repostar_combust_0, MPI_COMM_WORLD, &estado);
                     surtidores_tipo_0_en_uso++;
                  }
               }
               else {
                  MPI_Iprobe(MPI_ANY_SOURCE, etiq_comenzar_repostar_combust_1, MPI_COMM_WORLD, &flag, &estado);
                  if(flag > 0) {
                     if(surtidores_tipo_1_en_uso < surtidores[1]) {
                        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_comenzar_repostar_combust_1, MPI_COMM_WORLD, &estado);
                        surtidores_tipo_1_en_uso++;
                     }
                  }
                  else {
                     MPI_Iprobe(MPI_ANY_SOURCE, etiq_comenzar_repostar_combust_2, MPI_COMM_WORLD, &flag, &estado);
                     if(flag > 0) {
                        if(surtidores_tipo_2_en_uso < surtidores[2]) {
                           MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_comenzar_repostar_combust_2, MPI_COMM_WORLD, &estado);
                           surtidores_tipo_2_en_uso++;
                        }
                     }
                     else
                        sleep_for( milliseconds(20) );
                  }
               }
            }
         }
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
      if ( id_propio == id_gasolinera )                     // Si coincide con el id de la gasolinera
         funcion_gasolinera();                              //   es la gasolinera
      else {                                                // Si no 
         int tipo_combustible = aleatorio<0,2>();           //   es un coche        
         funcion_coche( id_propio, tipo_combustible );
      }
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