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
// Monitor, semántica SU, múltiples fumadores y un estanquero

class Estanco : public HoareMonitor
{
 private:
 int                        // variables permanentes
   mostrador;               // ventanilla en la que obtendremos el ingrediente correspondiente
 
 CondVar                    // colas condicion:
   mostrador_vacio,                       // cola donde espera el estanquero a que el mostrador
                                          // esté vacío para colocar un ingrediente
   ingrediente_obtenido[num_fumadores] ;  // cola donde espera el fumador a que el mostrador
                                          // tenga un ingrediente y que ese ingrediente coincida
                                          // con su número de fumador

 public:                    // constructor y métodos públicos
   Estanco(  ) ;           // constructor
   void obtener_ingrediente(int fumador);     // el fumador obtiene el ingrediente
   void poner_ingrediente( int ingrediente ); // el estanquero coloca el ingrediente en el mostrador
   void esperar_recogida_ingrediente();       // el estanquero espera a que el mostrador esté vacío
};

//-------------------------------------------------------------------------
// Constructor

Estanco::Estanco()
{
   mostrador = -1;
   mostrador_vacio = newCondVar();
   for(int i = 0; i < num_fumadores; i++)
      ingrediente_obtenido[i] = newCondVar();
}

//-------------------------------------------------------------------------
// El fumador correspondiente obtiene el ingrediente colocado por el estanquero

void Estanco::obtener_ingrediente(int fumador)
{
   // CONDICIÓN DE ESPERA: Hasta que el ingrediente no sea el que 
   //                      necesita el fumador, se queda bloqueado
   if(mostrador != fumador)
      ingrediente_obtenido[fumador].wait();

   //Comprobamos condición y actualizamos
   assert(0 <= mostrador && mostrador < num_fumadores);
   mostrador = -1; // Se retira el ingrediente y el mostrador se queda vacío

   // Notificamos que el mostrador se encuentra vacio
   mostrador_vacio.signal();
}

//-------------------------------------------------------------------------
// El fumador correspondiente obtiene el ingrediente colocado por el estanquero

void Estanco::poner_ingrediente(int ingrediente)
{
   //Comprobamos condición y actualizamos
   assert(0 <= ingrediente && ingrediente < num_fumadores);
   mostrador = ingrediente; // Se coloca el ingrediente en el mostrador

   //Notificamos al fumador correspondiente que su ingrediente está disponible
   ingrediente_obtenido[mostrador].signal();
}

//-------------------------------------------------------------------------
// El estanquero espera a que el mostrador esté vacío para seguir colocando ingredientes
void Estanco::esperar_recogida_ingrediente()
{
   // CONDICIÓN DE ESPERA: Si el mostrador no está vacío se espera
   if(mostrador != -1)
      mostrador_vacio.wait();
}

// *****************************************************************************
// Funciones de las hebras

//----------------------------------------------------------------------
// Función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
   int i;
   
   while(true){
      i = producir_ingrediente();
      monitor->poner_ingrediente(i);
      monitor->esperar_recogida_ingrediente();
   }
}

//----------------------------------------------------------------------
// Función que ejecuta la hebra del fumador
void funcion_hebra_fumador( MRef<Estanco> monitor, int num_fumador )
{
   assert(0 <= num_fumador && num_fumador < num_fumadores);

   while(true){
      monitor->obtener_ingrediente(num_fumador);
      fumar(num_fumador);
   }
}

// *****************************************************************************
// Función principal

int main()
{
   // Comprobación de las condiciones
   assert(num_fumadores > 0);
      
   // Declaración de las hebras y del monitor
   thread estanquero;                         // Hebra del estanquero
   thread fumadores[num_fumadores];           // Vector de hebras de los fumadores
   MRef<Estanco> monitor = Create<Estanco>(); // Monitor SU

   cout << "--------------------------------------" << endl
        << "Problema de los fumadores (Monitor SU)" << endl
        << "--------------------------------------" << endl
        << flush ;

   // Lanzamiento de las hebras
   estanquero = thread(funcion_hebra_estanquero, monitor);

   for(int i = 0; i < num_fumadores; i++)
      fumadores[i] = thread(funcion_hebra_fumador, monitor, i);

   // Sincronización entre las hebras
   estanquero.join();
   for(int i = 0; i < num_fumadores; i++)
      fumadores[i].join();

   return 0;
}
