/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 * 
 *  Autores: Zhong Hao Lin Chen, Alvaro Barroso Mato
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"

#include "string.h"

#define NO_RECURSIVO 0
#define RECURSIVO 1

/*
* Variable global que indica el tamano del buffer
* de caracteres leidos.
*/
#define VACIO -2
#define LLENO -1

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

//Objetivo parcial 3
typedef struct {
        //El nombre del mutex
        char *nombre;
	//Indica si es recursivo o no
	int tipo;          
	int ocupado;
	int procesos[MAX_PROC];             //Procesos con el mutex abierto	
	int procesosBloqueados[MAX_PROC];   //Procesos bloqueados en el mutex
	
	//Muestra el proceso propietario del mutex
	int propietario;
	//Indica numero de procesos en el mutex
	int procesos_mutex;
        //Positivo numero de veces bloqueado, 0 que es libre y negativo error	
	int bloqueado;	
}mutex;

typedef struct {
	int descript;
	int libre;	
} tipo_descriptor;

typedef struct BCP_t {
        int id;				/* ident. del proceso */
        int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	/* copia de regs. de UCP */
        void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */
        //Objetivo 2, plazo que falta para que se despierte un proceso  
        unsigned int plazo; 
	//Objetivo 4, tiempo que le queda a la actual rodaja
	unsigned int rodaja;	
	//unsigned int segs;		/* segundos que permance dormido el proceso*/
	int replanificacion;	        /* booleano para saber cuando hay que hacer un cambio de contexto involuntario */

	//Numero de veces interrupido en sistema						 
	int veces_sistema;	
	//Numero de veces interrupido en usuario
	int veces_usuario;		 			
	
	//Objetivo parcial 3, numero de mutex
	int numeroMutex;
	mutex *array_mutex_proceso[NUM_MUT_PROC];
	tipo_descriptor descriptores[NUM_MUT_PROC];
	
	//Indica que esta bloqueado por crear mutex 
	int blocCreandoMutex;
	//Indica el mutex que tiene bloqueado al proceso
	char *bloqueadoPorMutex;  
	
        //Inicio de bloqueo
	int instante_bloqueo;		
        //Segundos bloqueados
	int seg_bloqueo;
	
	//Objetivo parcial 5
	//Indica que esta bloqueado por lectura de caracter
	int blocLectura;
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en semáforo, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;

	
/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

//Objetivo 2
//Variable global que representa la cola de procesos listos
lista_BCPs lista_listos= {NULL, NULL};
//Variable global que representa la cola de procesos dromidos
lista_BCPs lista_dormidos = {NULL, NULL};

//Objetivo 3
//Variable global que representa al la lista de colas al crear el mutex
lista_BCPs lista_de_mutex = {NULL, NULL};
//Variable global para el lock y unlock de la lista de procesos bloqueados 
lista_BCPs lista_de_bloqueados = {NULL, NULL};


//Objetivo parcial 2, las dintintas variables
//Variable global que indica el numero de interrupciones de reloj producidas desde el arranque del sistema
int n_interrup;
//Variable global que indica el nivel previo de interrupción ante un cambio en el nivel de interrupcion
int nivel_anterior;
//Variable global que indica si estamos en modo sistema en una determinada zona
//int accede = 0;
//Variable global para veces init_reloj
//int numTicks = 0;


//Objetivo 3
//Array de mutex
mutex array_mutex[NUM_MUT];
//variable que indica el numero de mutex que hay
//int mutexExistentes = 0;

//Objetivo 4
//Variable global que indica si hay un cambio de contexto pendiente
int replanificacion_pendiente=0;

//Variable global para acceso a memoria por usuario
//int accesoMem = 0;


//Objetivo parcial 5
//Buffer de caracteres procesados del terminal
char Buffer[TAM_BUF_TERM];
//Variable global que indica el número de caracteres en el buffer
int BufferChar = 0;



/*
 * Define cuántas veces se ha detectado que el proceso ejecuta en modo
 * sistema y cuantas veces en modo usuario
 */
typedef struct tiempo_ejecucion {
    int usuario;
    int sistema;
} tiempo_ejecucion;

/*
 *
 * Definición del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;


/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();

//Objetivo parcial 1, Inclusion de una llamada simple
int obtener_id_pr();

//Objetivo parcial 2
int dormir(); 

//Objetivo parcial 3, todas las funciones del mutex
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock();
int sis_unlock();
int sis_cerrar_mutex();

//Objetivo parcial 5
int leer_caracter();


/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
/*objetivo parcial 1,incluir ultima posicion de la tabal*/     {obtener_id_pr},
//Objetivo parcial 2  
{dormir},
//Objetivo parcial 3
{sis_crear_mutex},
{sis_abrir_mutex},
{sis_lock},
{sis_unlock},
{sis_cerrar_mutex},
//Objetivo 5
{leer_caracter}
}; 

#endif /* _KERNEL_H */

