// Nombre: Juan Manuel Rodríguez Gómez
// DNI: 49559494Z
// Grupo: SCD 3, Doble Grado Ingeniería Informática y Matemáticas

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo1-compr.cpp
// Modificación del primer ejemplo de ejecutivo cíclico:
//
// Ahora cada vez que acaba un ciclo secundario, se
// informa del retraso del instante final actual respecto al instante
// final esperado.
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  250  100
//   B  250   80
//   C  500   50
//   D  500   40
//   E 1000   20
//  -------------
//
//  Se considera que D_i = T_i para toda tarea.
//
//  El ciclo principal dura TM = mcm(250, 250, 500, 500, 100) = 1000 ms
//
//  Según lo visto en teoría, para la obtención de un buen valor del período
//  del ciclo secundario, Ts, este debe cumplir lo siguiente:
//
//     1) TM = k * Ts, siendo k un número natural
//        (El período del ciclo secundario será un divisor del período del ciclo secundario)
//
//     2) máx(C1, C2, C3, C4, C5) <= Ts <= mín(D1, D2, D3, D4, D5), luego, 
//        100 ms <= Ts <= 250 ms  
//
//  Planificación (con Ts == 250 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D E  | A B C   | A B D  |
//  *---------*----------*---------*--------*
//
//
// Historial:
// Creado en Diciembre de 2017
// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// Tipo para duraciones en segundos y milisegundos, en coma flotante:
//typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// Tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// Tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds( 80) );  }
void TareaC() { Tarea( "C", milliseconds( 50) );  }
void TareaD() { Tarea( "D", milliseconds( 40) );  }
void TareaE() { Tarea( "E", milliseconds( 20) );  }

// -----------------------------------------------------------------------------
// Implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario (en unidades de milisegundos, enteros)
   const milliseconds Ts_ms( 250 );

   // ini_sec = instante de inicio de la iteración actual del ciclo secundario
   time_point<steady_clock> ini_sec = steady_clock::now();

   while( true ) // Ciclo principal
   {
      cout << endl
           << "---------------------------------------" << endl
           << "Comienza iteración del ciclo principal." << endl ;

      for( int i = 1 ; i <= 4 ; i++ ) // Ciclo secundario (4 iteraciones)
      {
         cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

         switch( i )
         {
            case 1 : TareaA(); TareaB(); TareaC();           break ;
            case 2 : TareaA(); TareaB(); TareaD(); TareaE(); break ;
            case 3 : TareaA(); TareaB(); TareaC();           break ;
            case 4 : TareaA(); TareaB(); TareaD();           break ;
         }

         // Calcular el siguiente instante de inicio del ciclo secundario
         ini_sec += Ts_ms ;

         // Esperar hasta el inicio de la siguiente iteración del ciclo secundario
         sleep_until( ini_sec );

         // Calculamos el instante final actual
         // Para ello, usamos una variable del tipo "time_point<steady_clock>""
         time_point<steady_clock> fin_sec = steady_clock::now();

         // Calculamos el retraso del instante final actual respecto al esperado
         // Para ello, usamos una variable del tipo "steady_clock::duration"
         steady_clock::duration retraso = fin_sec - ini_sec;
         
         // Si no ha habido retraso, se muestra por pantalla que no ha habido retraso
         if(milliseconds_f(retraso).count() <= 0)
            cout << endl << "   No ha habido retraso en la iteración " << i << " del ciclo secundario." << endl;
         // Si ha habido retraso, se muestra por pantalla dicho retraso (en milisegundos)
         else
            cout << endl << "   Ha habido retraso en la iteración " << i << " del ciclo secundario de " << milliseconds_f(retraso).count() << " milisegundos." << endl;
      }
   }
}