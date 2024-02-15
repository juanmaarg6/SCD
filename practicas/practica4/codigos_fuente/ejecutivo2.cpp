// Nombre: Juan Manuel Rodríguez Gómez
// DNI: 49559494Z
// Grupo: SCD 3, Doble Grado Ingeniería Informática y Matemáticas

// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo2.cpp
// Implementación del segundo ejemplo de ejecutivo cíclico:
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  500  100
//   B  500  150
//   C 1000  200
//   D 2000  240
//  -------------
//
//  Se considera que D_i = T_i para toda tarea.
//
//  El ciclo principal dura TM = mcm(500, 500, 1000, 2000) = 2000 ms
//
//  Según lo visto en teoría, para la obtención de un buen valor del período
//  del ciclo secundario, Ts, este debe cumplir lo siguiente:
//
//     1) TM = k * Ts, siendo k un número natural
//        (El período del ciclo secundario será un divisor del período del ciclo secundario)
//
//     2) máx(C1, C2, C3, C4) <= Ts <= mín(D1, D2, D3, D4), luego, 
//        240 ms <= Ts <= 500 ms  
//
//
//  Planificación (con Ts == 500 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D    | A B C   | A B    |
//  *---------*----------*---------*--------*
//
//
// Historial:
// Creado en Diciembre de 2017
// -----------------------------------------------------------------------------

//
// Se nos pide contestar a las siguientes preguntas:
//     
//     PREGUNTA 1: ¿Cuál es el mínimo tiempo de espera que queda al final
//                  de las iteraciones del ciclo secundario con tu solución?
//
//     RESPUESTA 1:
//
//        - En la 1ª iteración del ciclo secundario se ejecutan [A, B, C],
//          consumiendo un total de T1 = 100 ms + 150 ms + 200 ms = 450 ms.
//
//        - En la 2ª iteración del ciclo secundario se ejecutan [A, B, D],
//          consumiendo un total de T2 = 100 ms + 150 ms + 240 ms = 490 ms.
//
//        - En la 3ª iteración del ciclo secundario se ejecutan [A, B, C],
//          consumiendo un total de T3 = 100 ms + 150 ms + 200 ms = 450 ms.
//
//        - En la 4ª iteración del ciclo secundario se ejecutan [A, B],
//          consumiendo un total de T4 = 100 ms + 150 ms = 250 ms.
//
//        Por tanto, el mínimo tiempo de espera es T_mínimo_espera = Ts - máx(T1, T2, T3, T4) = 500 ms - 490 ms = 10 ms
//
//
//     PREGUNTA 2: ¿Sería planificable si la tarea D tuviese un tiempo de cómputo de 250 ms?
//
//
//     RESPUESTA 2:
//        
//        Según la planificación elegida en este caso, sí sería planificable. Sin embargo, no habría
//        tiempo de espera en la 2ª iteración del ciclo seecundario, luego, habría que ir con cuidado
//        ya que cualquier espera adicional provocaría un pequeño retraso con respecto a la siguiente iteración.
//        Esto sería muy arriesgado en Sistemas de Tiempo Real (aquí estamos haciendo una simulación
//        en Sistemas de Propósito General).
//
//        Vemos que para D == 250 ms, se produce un retraso en la iteración 2 del ciclo secundario 
//        ( ya que es ahí donde se ejecuta TareaD() ) de aproximadamente 1'5 ms. Para D == 260 ms,
//        el retraso pasa a ser de 12 ms aproximadamente (hay un gran aumento del retraso).

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
void TareaB() { Tarea( "B", milliseconds(150) );  }
void TareaC() { Tarea( "C", milliseconds(200) );  }
void TareaD() { Tarea( "D", milliseconds(240) );  }

// -----------------------------------------------------------------------------
// Implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario (en unidades de milisegundos, enteros)
   const milliseconds Ts_ms( 500 );

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
            case 1 : TareaA(); TareaB(); TareaC();   break ;
            case 2 : TareaA(); TareaB(); TareaD();   break ;
            case 3 : TareaA(); TareaB(); TareaC();   break ;
            case 4 : TareaA(); TareaB();             break ;
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