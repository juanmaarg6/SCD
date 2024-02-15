// Nombre: Juan Manuel Rodríguez Gómez
// DNI: 49559494Z
// Grupo: SCD 3, Doble Grado Ingeniería Informática y Matemáticas

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-cam.cpp
// Implementación del problema de los filósofos (con camarero).
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
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
   num_filosofos = 5,                   // Número de filósofos 
   num_filo_ten  = 2*num_filosofos,     // Número de filósofos y tenedores 
   num_procesos  = num_filo_ten + 1 ;   // Número de procesos total (hay filo, ten. y el camarero)

const int 
   id_camarero = num_procesos - 1;  // Identificador del camarero

const int 
   etiq_coger_tenedor = 1,    // Etiqueta para mensajes de coger tenedor
   etiq_soltar_tenedor = 2,   // Etiqueta para mensajes de soltar tenedor
   etiq_sentarse_mesa = 3,    // Etiqueta para mensajes de sentarse en la mesa
   etiq_levantarse_mesa = 4;  // Etiqueta para mensajes de levantarse de la mesa

const int 
   max_num_filo_sentados = 4; // Máximo número de filósofos que pueden estar sentados en la mesa


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

void funcion_filosofos( int id )
{
   int id_ten_izq = (id+1)              % num_filo_ten, // Identificador tenedor izq.
       id_ten_der = (id+num_filo_ten-1) % num_filo_ten; // Identificador tenedor der.

  while ( true )
  {   
      // Sentarse en la mesa
      cout <<"Filósofo " <<id << " solicita sentarse en la mesa" <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_camarero, etiq_sentarse_mesa, MPI_COMM_WORLD);

      // Coger tenedores
      cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_ten_izq, etiq_coger_tenedor, MPI_COMM_WORLD);

      cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_ten_der, etiq_coger_tenedor, MPI_COMM_WORLD);

      // Comer
      cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
      sleep_for( milliseconds( aleatorio<10,100>() ) );

      // Soltar tenedores
      cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_ten_izq, etiq_soltar_tenedor, MPI_COMM_WORLD);

      cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_ten_der, etiq_soltar_tenedor, MPI_COMM_WORLD);

      // Levantarse de la mesa
      cout <<"Filósofo " <<id << " solicita levantarse de la mesa" <<endl;
      MPI_Ssend( &id,  1, MPI_INT, id_camarero, etiq_levantarse_mesa, MPI_COMM_WORLD);

      // Pensar
      cout << "Filosofo " << id << " comienza a pensar" << endl;
      sleep_for( milliseconds( aleatorio<10,100>() ) );
   }
}

// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
   int valor,            // Valor recibido
       id_filosofo ;     // Identificador del filósofo
   MPI_Status estado ;   // Metadatos de las dos recepciones

  while ( true )
  {
      // Tenedor cogido por filósofo
      MPI_Recv (&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_coger_tenedor, MPI_COMM_WORLD, &estado);
      id_filosofo = estado.MPI_SOURCE;
      cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

      // Tenedor liberado por filósofo
      MPI_Recv (&valor, 1, MPI_INT, id_filosofo, etiq_soltar_tenedor, MPI_COMM_WORLD, &estado);
      cout <<"Ten. "<< id<< " ha sido liberado por filo. " <<id_filosofo <<endl ;
  }
}

// ---------------------------------------------------------------------

void funcion_camarero()
{
   int valor,                        // Valor recibido
       etiq_aceptada,                // Etiqueta aceptada del mensaje
       num_filosofos_sentados = 0;   // Número de filósofos sentados en la mesa
   
   MPI_Status estado;   // Metadatos de las dos recepciones

   while ( true ) 
   {
      // Si en la mesa ya se ha alcanzado el número máximo de filósofos que pueden estar sentados,
      // solo se admiten mensajes de levantarse de la mesa
      if(num_filosofos_sentados == max_num_filo_sentados)
         etiq_aceptada = etiq_levantarse_mesa;
      // Si en la mesa hay filósofos sentados pero todavía se puede sentar algún filósofo más,
      // se admiten mensajes tanto de sentarse como de levantarse de la mesa
      else
         etiq_aceptada = MPI_ANY_TAG;

      // No añadimos la opción if(num_filosofos_sentados == 0) then etiq_aceptada = etiq_sentarse_mesa;
      // ya que al no haber filósofos sentados nunca se va a recibir un mensaje para levantarse de la mesa.

      // El camarero recibe el mensaje con la etiqueta aceptada correspondiente
      MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptada, MPI_COMM_WORLD, &estado);

      // Procesamos el mensaje recibido

      // Gestionamos el número de filósofos que hay sentados en la mesa en cada momento
      // Mostramos por pantalla la aceptación del camarero de la petición de sentarse (o de levantarse) de la mesa por parte de un filósofo
      if(estado.MPI_TAG == etiq_sentarse_mesa) {
         num_filosofos_sentados++;
         cout << "Camarero acepta la peticion de filo. " << estado.MPI_SOURCE << " de sentarse en la mesa" << endl; 
      }
      
      if(estado.MPI_TAG == etiq_levantarse_mesa) {
         num_filosofos_sentados--;
         cout << "Camarero acepta la peticion de filo. " << estado.MPI_SOURCE << " de levantarse en la mesa" << endl; 
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
      if ( id_propio == id_camarero )     // Si coincide con el id del camarero
         funcion_camarero();              //   es el camarero
      else if(id_propio % 2 == 0)         // Si es par                  
         funcion_filosofos( id_propio );  //   es un filósofo
      else                                // Si es impar
         funcion_tenedores( id_propio );  //   es un tenedor

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