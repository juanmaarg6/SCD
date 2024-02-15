#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

const int num_fumadores = 3; // Número de fumadores

vector<Semaphore> ingr_disp;  // Vector de semáforos donde en la posición i hay un 1 si el ingrediente i está disponible en el mostrador y 0 si no lo está
Semaphore mostr_vacio(1);     // Semáforo que vale 1 si el mostrador está vacío y 0 si el mostrador está ocupado

//**********************************************************************
// Funciones del semáforo

//-------------------------------------------------------------------------
// Función que inicializa el vector "ingr_disp"

void iniciar_semaforos() 
{
   assert(num_fumadores > 0);
   for(int i = 0; i < num_fumadores; i++)
      ingr_disp.push_back(Semaphore(0));
}

//**********************************************************************
// Funciones simuladas

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatorio de la hebra

void fumar( int num_fumador )
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

// *****************************************************************************
// Funciones de las hebras

//----------------------------------------------------------------------
// Función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   int i;
   
   while(true) {
      i = producir_ingrediente();
   
      mostr_vacio.sem_wait();
      cout << "Puesto ingr.: " << i << endl;
      ingr_disp[i].sem_signal();
   }
}

//----------------------------------------------------------------------
// Función que ejecuta la hebra del fumador

void  funcion_hebra_fumador( int num_fumador )
{
   assert(0 <= num_fumador && num_fumador < num_fumadores);

   while(true) {

      ingr_disp[num_fumador].sem_wait();
      cout << "Retirado ingr.: " << num_fumador << endl;
      mostr_vacio.sem_signal();

      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   // Comprobación de las condiciones
   assert(num_fumadores > 0);

   // Inicialización de los semáforos
   iniciar_semaforos();
      
   // Declaración de las hebras
   thread estanquero;               // Hebra del estanquero
   thread fumadores[num_fumadores]; // Vector de hebras de los fumadores

   cout << "------------------------------------" << endl
        << "Problema de los fumadores (Semaforo)" << endl
        << "------------------------------------" << endl
        << flush ;

   // Lanzamiento de las hebras
   estanquero = thread(funcion_hebra_estanquero);
   for(int i = 0; i < num_fumadores; i++)
      fumadores[i] = thread(funcion_hebra_fumador, i);

   // Sincronización entre las hebras
   estanquero.join();
   for(int i = 0; i < num_fumadores; i++)
      fumadores[i].join();

   return 0;
}
