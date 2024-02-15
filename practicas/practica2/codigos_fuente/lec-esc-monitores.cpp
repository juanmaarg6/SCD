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

const int
   num_lectores = 3,
   num_escritores = 3;

mutex
   mtx;

//**********************************************************************
// Funciones simuladas

//----------------------------------------------------------------------
// Función de escribir
void escribir(int num_escritor)
{
    chrono::milliseconds duracion_escritura(aleatorio<20,200>());

    mtx.lock();
    cout << "[Escritor " << num_escritor << "]: Escribiendo datos... ("
         << duracion_escritura.count() << " milisegundos)" << endl;
    mtx.unlock();

    this_thread::sleep_for(duracion_escritura);
}

//----------------------------------------------------------------------
// Función de leer
void leer(int num_lector)
{
    chrono::milliseconds duracion_lectura(aleatorio<20,200>());

    mtx.lock();
    cout << "[Lector " << num_lector << "]: Leyendo datos... ("
         << duracion_lectura.count() << " milisegundos)" << endl;
    mtx.unlock();

    this_thread::sleep_for(duracion_lectura);
}

// *****************************************************************************
// Monitor, semántica SU, múltiples lectores y múltiples escritores
class Lec_Esc : public HoareMonitor
{
 private:
 int    // Variables permanentes:
   n_lec; // Número de lectores
 bool 
   escrib; // Comprueba que hay un escritor activo

 CondVar    // Colas condición:
   lectura,     // Cola de lectores
   escritura;   // Cola de escritores

 public:
   Lec_Esc();             // Constructor por defecto
   void ini_lectura();    // Función de inicio de lectura
   void fin_lectura();    // Función de fin de lectura
   void ini_escritura();  // Función de inicio de escritura
   void fin_escritura();  // Función de fin de escritura
};

//-------------------------------------------------------------------------
// Constructor

Lec_Esc::Lec_Esc()
{
  n_lec = 0;
  escrib = false;
  lectura = newCondVar();
  escritura = newCondVar();
}

//-------------------------------------------------------------------------
// Función de inicio de lectura

void Lec_Esc::ini_lectura()
{
  // Comprobación de si hay un escritor activo
  if(escrib)
    lectura.wait(); // El comienzo de la lectura espera

  // Registramos un lector más
  n_lec++;

  // Desbloqueamos en cadena los posibles lectores
  lectura.signal();
}

//-------------------------------------------------------------------------
// Función de fin de lectura
void Lec_Esc::fin_lectura()
{
  // Registramos un lector menos
  n_lec--;

  // Si es el último lector, desbloqueamos un escritor
  if(n_lec == 0)
    escritura.signal();
}

//-------------------------------------------------------------------------
// Función de inicio de escritura

void Lec_Esc::ini_escritura()
{
  // Comprobación de si se está leyendo o si hay otro escritor escribiendo
  if((n_lec > 0) or escrib)
    escritura.wait(); // El comienzo de la escritura espera
  
  // Registramos que un escritor se encuentra escribiendo
  escrib = true;
}

//-------------------------------------------------------------------------
// Función de fin de escritura

void Lec_Esc::fin_escritura()
{
  // Registramos que ya el escritor no está escribiendo
  escrib = false;

  // Si hay lectores esperando, despertar uno
  if(!lectura.empty())
    lectura.signal();
  // Si no hay lectores esperando, despertar un escritor
  else
    escritura.signal();
}

// *****************************************************************************
// Funciones de las hebras

//----------------------------------------------------------------------
// Función que ejecuta la hebra lectora

void funcion_hebra_lector(MRef<Lec_Esc> monitor, int num_lector)
{
  while(true){
    chrono::milliseconds retardo(aleatorio<20,200>());
    this_thread::sleep_for(retardo);
    
    monitor->ini_lectura();
    leer(num_lector);
    monitor->fin_lectura();
  }
}

//----------------------------------------------------------------------
// Función que ejecuta la hebra escritora

void funcion_hebra_escritor(MRef<Lec_Esc> monitor, int num_escritor)
{
  while(true){
    chrono::milliseconds retardo(aleatorio<20,200>());
    this_thread::sleep_for(retardo);

    monitor->ini_escritura();
    escribir(num_escritor);
    monitor->fin_escritura();
  }
}

// *****************************************************************************
// Función principal

int main()
{
  // Comprobación de las condiciones
  assert(0 < num_lectores && 0 < num_escritores);

  // Declaración de las hebras y del monitor
  thread lectores[num_lectores];
  thread escritores[num_escritores];
  MRef<Lec_Esc> monitor = Create<Lec_Esc>();

  cout << "------------------------------------------------" << endl
       << "Problema de los lectores-escritores (Monitor SU)" << endl
       << "------------------------------------------------" << endl
       << flush ;

  // Lanzamiento de las hebras
  for(int i = 0; i < num_lectores; i++)
    lectores[i] = thread(funcion_hebra_lector, monitor, i);

  for(int i = 0; i < num_escritores; i++)
    escritores[i] = thread(funcion_hebra_escritor, monitor, i);

  // Sincronización entre las hebras
  for(int i = 0; i < num_lectores; i++)
    lectores[i].join();

  for(int i = 0; i < num_escritores; i++)
    escritores[i].join();

  return 0;
}