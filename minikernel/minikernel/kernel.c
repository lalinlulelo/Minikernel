/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Autores: Zhong Hao Lin Chen, Alvaro Barroso Mato
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */
#include "string.h"

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Función que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Función que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){
	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al mínimo el nivel de interrupción mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
		
	//Asigna el tiempo de la rodaja al proceso
	BCP *proceso = lista_listos.primero;
	proceso->rodaja = TICKS_POR_RODAJA;
	
	return lista_listos.primero;
}


//Objetivo parcial 4, round robin
//Función que realiza un cambio de proceso ya sea voluntario o involuntario
static void cambio_proc(lista_BCPs *lista_destino) {
        //Declaramos el puntero al proceso anterior un contexto auxiliar y un nivel
	BCP * p_proc_anterior;
	contexto_t *contexto_aux;
	int nivel;

	//El proceso anterior lo igualamos al proceso actual y fijamos el nivel a 3
	p_proc_anterior=p_proc_actual;
	nivel=fijar_nivel_int(NIVEL_3);

	//El proceso no sigue. En caso de replanificación pendiente desactivar proceso
	replanificacion_pendiente=0;
	eliminar_primero(&lista_listos);

	//Si se ha especificado una lista destino para el BCP, se inserta.
	if (lista_destino){
		insertar_ultimo(lista_destino, p_proc_anterior);
		if (lista_destino != &lista_listos){
			p_proc_actual->estado=BLOQUEADO;
		}
		printk("identificador del proceso:  %d\n",p_proc_actual->id);
	}
		
	p_proc_actual=planificador();

	//Se asigna una rodaja completa al proceso que va a ejecutar
	p_proc_actual->rodaja=TICKS_POR_RODAJA;

	//Si el proceso ya ha terminado, no se salva y se libera la pila 
	if (p_proc_anterior->estado==TERMINADO){
		contexto_aux=NULL;
		liberar_pila(p_proc_anterior->pila);
	}
	else{ //En caso contrario al contexto auxiliar se le iguala la direccion del registro del proceso anterior
		contexto_aux=&(p_proc_anterior->contexto_regs);
	}
	cambio_contexto(contexto_aux, &(p_proc_actual->contexto_regs));

	fijar_nivel_int(nivel);
}


/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;
	
	if(p_proc_actual->numeroMutex!=0){
	       sis_cerrar_mutex(p_proc_actual);
	}
	       
	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
        
        //fijar_nivel_int(NIVEL_3);

	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	
        return; /* no debería llegar aqui */
}


/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char car;
	
	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

	//Objetivo parcial 5 
        //Si el buffer no está lleno introduce el caracter nuevo
	if(BufferChar < TAM_BUF_TERM){
		Buffer[BufferChar] = car;
		BufferChar++;		

		//Desbloquea el primer proceso bloqueado por lectura
		BCP *proceso_bloqueado = lista_de_bloqueados.primero;
	        
		//Al principio esta bloqueado
		int desbloqueado = 0;
		if (proceso_bloqueado != NULL){
			if(proceso_bloqueado->blocLectura == 1){
				//Desbloquea el proceso
				desbloqueado = 1;
				//El proceso bloqueado ahora esta listo
				proceso_bloqueado->estado = LISTO;
				proceso_bloqueado->blocLectura = 0;
				//Cambio de contexto
				int nivel_interrupciones = fijar_nivel_int(NIVEL_3);
				eliminar_elem(&lista_de_bloqueados, proceso_bloqueado);
				insertar_ultimo(&lista_listos, proceso_bloqueado);
				fijar_nivel_int(nivel_interrupciones);
			}
		}
		
		//Mientras no este desbloqueado y el ultimo de la lista de dormidos no este bloqueado
		while(desbloqueado != 1 && proceso_bloqueado != lista_de_bloqueados.ultimo){
			proceso_bloqueado = proceso_bloqueado->siguiente;
			if(proceso_bloqueado->blocLectura == 1){
				//Desbloquea el proceso
				desbloqueado = 1;
				//El proceso bloqueado ahora esta listo
				proceso_bloqueado->estado = LISTO;
				proceso_bloqueado->blocLectura = 0;
				//Cambio de contexto
				int nivel_interrupciones = fijar_nivel_int(NIVEL_3);
				eliminar_elem(&lista_de_bloqueados, proceso_bloqueado);
				insertar_ultimo(&lista_listos, proceso_bloqueado);
				fijar_nivel_int(nivel_interrupciones);
			}
		}
	}	
	
	return;
}

//Objetivo parcial 4, Round robin
//Función auxiliar que actualiza la rodaja y si detecta su terminación activa una interrupción software
static void ajustar_rodaja() {
	//Si el proceso no esta listo para ejecutar, no se actualiza la rodaja 
	if (p_proc_actual->estado == LISTO) {
		p_proc_actual->rodaja--;
		if (p_proc_actual->rodaja==0) {
			replanificacion_pendiente=1;
			activar_int_SW();
		}
	}
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){
	printk("-> TRATANDO INT. DE RELOJ\n");
	
	//Objetivo parcial 3
	ajustar_rodaja();
	
        //Objetivo parcial 2
        ajustar_dormidos();
	
        return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){

	printk("-> TRATANDO INT. SW\n");
	
	cambio_proc(&lista_listos);
	/*
	//Objetivo parcial 3
	//Variable local para id del proceso que va a int sw
        int idABloquear = 0;
	// Interrupcion SW de planificacion
	// Comprueba que proceso en ejecución es el que se quiere bloquear
	if(idABloquear == p_proc_actual->id){
		//Pone el proceso ejecutando al final de la cola de listos
		BCP *proceso = lista_listos.primero;
		int nivel_interrupciones = fijar_nivel_int(NIVEL_3);
		eliminar_elem(&lista_listos, proceso);
		insertar_ultimo(&lista_listos, proceso);
		fijar_nivel_int(nivel_interrupciones);

		//Cambio de contexto por int sw de planificación
		BCP *p_proc_bloqueado = p_proc_actual;
		p_proc_actual = planificador();
		cambio_contexto(&(p_proc_bloqueado->contexto_regs), &(p_proc_actual->contexto_regs));
	}
	*/

	return;
}


/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;
	int nivel;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		//Inicialmente se asigna una rodaja completa para round robin
		p_proc->rodaja=TICKS_POR_RODAJA;

		/* Dado que la ruina de int. de reloj también manipula la lista
	   	   de listos, se prohú~en las int. en este fragmento */
		nivel=fijar_nivel_int(NIVEL_3);
		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		fijar_nivel_int(nivel);
		error= 0;
	}
	else
		error = -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}


/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no debería llegar aqui */
}

//Objetivo parcial 1: Inclusion de una llamada simple
//codigo de la nueva funcion
int obtener_id_pr(){
	
        printk("-> ID DEL PROCESO ACTUAL %d\n", p_proc_actual->id);

        return p_proc_actual->id;
}

/*
 * 
 * Todo el objetivo 2 de bloquear un proceso durante un tiempo
 *
 */

/*
//Funcion que bloquea el proceso que se esta ejecutando actualmente
static void bloquear(lista_BCPs *lista){
        BCP * p_proc_anterior; //Declaramos un puntero al proceso anterior
        int nivel;

        p_proc_actual->estado=BLOQUEADO; //Bloqueamos el proceso actual
        p_proc_anterior = p_proc_actual;  //y volvemos al proceso anterior

        nivel=fijar_nivel_int(NIVEL_3);   //Fijamos el nivel

        //El proceso no sigue, si hay alguna replanificacion pendiente hay que desactivarla
        eliminar_primero(&lista_listos);

        insertar_ultimo(lista, p_proc_anterior);  //Insertamos en la ultima posicion de la lista el proceso anterior
        
	//Al proceso actual le igualamos al planificador
        p_proc_actual = planificador;   

        cambio_contexto(&(p_proc_anterior->contexto_regs),&(p_proc_actual->contexto_regs));
        
        fijar_nivel_int(nivel);
}

//Funcion para desbloquear un proceso dormido
static void desbloquear(BCP * proc, lista_BCPs *lista){
        int nivel;
        
	//Ponemos el proceso en estado listo
        proc->estado=LISTO;
        nivel=fijar_nivel_int(NIVEL_3);
	//De la lista espera eliminamos el proceso ya listo
        eliminar_elem(lista, proc);
	//Y insertamos en la lista de procesos listos el anterior proceso
        insertar_ultimo(&lista_listos, proc);
        fijar_nivel_int(nivel);
}
*/

//Funcion auxiliar que decrementa el tiempo de espera y cuando llega a 0 lo desbloquea
void ajustar_dormidos() {
        /*
        BCP * p_aux;
        BCP * p_sig;

        p_aux=lista_dormidos.primero;
        while (p_aux) {
            p_sig=p_aux->siguiente;
            if (--p_aux->plazo==0)
               desbloquear(p_aux, &lista_dormidos);
            p_aux=p_sig;
        }
        */
        
       	//Tiempo de los procesos
	n_interrup++;
	//Asignamos las interrupciones al usuario o al sistema
	if(lista_listos.primero != NULL){
		if(viene_de_modo_usuario()){
			p_proc_actual->veces_usuario++;
		}
		else{
			p_proc_actual->veces_sistema++;
		}
	}

	//Tratamos los procesos dormidos 
	if(lista_dormidos.primero != NULL){
		BCP * proc_dormido;
		BCP * dorm_aux;

		proc_dormido = lista_dormidos.primero;
                //Mientras no haya procesos dormidos
		while(proc_dormido != NULL){
			proc_dormido->plazo--;
			//Si termina la espera de algun proceso
			if(proc_dormido->plazo <= 0){
				//Se vuelve a ajustar la lista dormidos
				proc_dormido->estado = LISTO;

				if(proc_dormido->siguiente != NULL){
					//El auxiliar apunta al siguiente de los procesos dormidos
					dorm_aux = proc_dormido->siguiente;
					//Elimina los dormidos de la lista dormidos y los inserta en lo ultimo de la lista listos
					eliminar_elem(&lista_dormidos, proc_dormido);
					insertar_ultimo(&lista_listos, proc_dormido);
					proc_dormido = dorm_aux;
				}
				else{
					eliminar_elem(&lista_dormidos, proc_dormido);
					insertar_ultimo(&lista_listos, proc_dormido);
				}
				proc_dormido = proc_dormido->siguiente;
			}
		}
	}     
}

//Objetivo parcial 2: bloquea al proceso un plazo de tiempo
int dormir(){	
        unsigned int segundos;
        segundos = (unsigned int)leer_registro(1);
       
        /*
        //Al proceso actual le pongo los segundos en ticks que quiero que este dormido
        p_proc_actual->plazo=segundos*TICK;
        
        //Bloque el proceso 
        //bloquear(&lista_dormidos);
	
	//se bloquea el proceso
	p_proc_actual->estado=BLOQUEADO;
	cambio_proceso(&lista_dormidos);
	
	printk("-> EL PROCESO ACTUAL HA DORMIDO %d\n", (segundos*TICK));
	
	*/

  	BCP * p_proc_anterior;
 	//Bloqueamos el proceso e insertamos el tiempo de bloqueo
 	p_proc_actual->estado = BLOQUEADO;
 	p_proc_actual->plazo = segundos*TICK;
	
 	p_proc_actual->replanificacion = 0;
 	nivel_anterior = fijar_nivel_int(NIVEL_3);
	
        //Lo quitamos de la lista de procesos listos y lo metemos en la de dormidos
 	eliminar_primero(&lista_listos);
 	insertar_ultimo(&lista_dormidos, p_proc_actual);
 	//Realizar cambio de contexto
 	p_proc_anterior = p_proc_actual;
 	p_proc_actual = planificador();

 	printk("Duerme desde el proceso %d hasta %d\n", p_proc_anterior->id, p_proc_actual->id);

 	//Restauramos el contexto de nuestro nuevo proc_actual
 	cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
 	//Recuperamos el nivel anterior de interrupciones
 	fijar_nivel_int(nivel_anterior);
	
	printk("-> EL PROCESO ACTUAL %d HA DORMIDO %u\n", p_proc_actual->id, p_proc_actual->plazo);

        return 0; //Llamada no da error
}

 /*
  * 
  * Tratamiento de la llamada al sistema tiempos_proceso
  *
 */
 int sis_tiempos_proceso(){
 	struct tiempo_ejecucion *t_ejec;
 	t_ejec = (struct tiempo_ejecucion *)leer_registro(1);
 	
 	if(t_ejec != NULL){
 		nivel_anterior = fijar_nivel_int(3);
 		t_ejec->usuario = p_proc_actual->veces_usuario;
 		t_ejec->sistema = p_proc_actual->veces_sistema;
 		//accede = 1;
 		fijar_nivel_int(nivel_anterior);
 	}
 	return n_interrup;
} 

/*
*
* Nos hemos creado varias funciones auxiliares que realizan las operaciones del objetivo parcial 3 del mutex
*
*/

//Funcion auxiliar que nos dice si el proceso actual tiene algun descriptor o no.
//Devuelve el descriptor si existe y en caso contrario un -1
int tiene_descriptor(){
	int tiene = 0;
	int n = 0;
	//Mientras el numero auxiliar sea menor al maximo numero de mutex que pueda tener un proceso y no tenga descriptor seguira ejecutandose el bucle
	while((n < NUM_MUT_PROC) && (!tiene)){
		//Comprueba que haya un decriptor 
		if(p_proc_actual->descriptores[n].libre == 0){
			tiene = 1;
			//printk("El proceso actual tiene un descriptor.\n");
		}
		//Aumentamos el contador de numeros
		n++;
	}
	
	//En caso de haber descriptor devuelve pues el numero de donde esta el descriptor
	if(tiene){
	        printk("El proceso actual %d tiene un descriptor.\n", (n-1));
		return n-1;
	}else{
	 //En caso de que no haya decriptor devuelve un numero negativo -1
	        printk("No tiene descriptor. \n");
		return -1;
	}
}

//Funcion auxiliar que nos dice si hay algun descriptor libre en el proceso actual
//Devuelve que esta lleno si no hay libre y un n-1 en caso contrario
int libre(){
	int tiene_libre = 0;
	int n = 0;
        //Mientras el numero sea menor al maximo numero de mutex que pueda tener un proceso y no tenga descriptor seguira ejecutandose el bucle
	while((n < NUM_MUT) && (!tiene_libre)){
		//Comprobamos si en el array de mutex segun la posicion los numeros de procesos son menores o iguales a 0 entonces es que tiene ya un descriptor
		if(array_mutex[n].procesos_mutex <= 0){
			tiene_libre = 1;
			printk("Hay un descriptor libre en proceso de mutex %d actual.\n",array_mutex[n].procesos_mutex);
		}
		//Aumentamos el contador de numeros
		n++;
	}
	
	if(!tiene_libre){
	        printk("Esta lleno y no queda descriptor libre.\n");
		return LLENO;
	}else{
	        printk("Descriptor libre en %d .\n", (n-1));
	        return n-1;
	}
}

//Funcion auxiliar que comprueba si existe o no un mutex dado
//Devuelve 1 en caso de encontrarlo y 0 en caso contrario
int existe_mutex(char*nom){
	int existe = 0;
	int n = 0;

	//Mientras el numero sea menor al maximo numero de mutex que pueda tener un proceso y no tenga descriptor seguira ejecutandose el bucle
	while((n < NUM_MUT) && (!existe)){
		//Buscamos en indices que tengan mutex
		if(array_mutex[n].procesos_mutex > 0) {
			//Verificamos si el nombre coincide
			if(strcmp(nom, array_mutex[n].nombre)){
				existe = 1;
				printk("El nombre introducido coincide con el del mutex.\n");
			}
		}
		//Aumentamos el contador de numeros
		n++;
	}
	
	if(existe){
	        printk("El mutex buscado por el nombre existe.\n");
		return 1;
	}else{
	        printk("El mutex buscado por el nombre no existe.\n");
		return 0;
	}
}

//Funcion auxiliar que comprueba la posicion de un mutex dado previamente un nombre
//Devuelve la posicion en caso de encontrarlo y -1 en caso contrario
int pos_mutex(char*nombre){
	int existe = 0;
	int n = 0;

	//Mientras el numero sea menor al maximo numero de mutex que pueda tener un proceso y no tenga descriptor seguira ejecutandose el bucle
	while((n < NUM_MUT) && (!existe)){
		//seleccionamos solo aquellos indices con un mutex
		if (array_mutex[n].procesos_mutex > 0){
			//Comprobamos si coincide el nombre del mutex con el nombre escrito en la funcion
			if(strcmp(nombre, array_mutex[n].nombre) == 0){
				existe = 1;
			}
		}
		//Aumentamos el contador de numeros
		n++;
	}
	
	if(!existe){
	        printk("No hay posicion porque no existe.\n");
		return -1;
	}else{
	        printk("La posicion del mutex es %d .\n",(n-1));
		return n-1;
	}
}


 
/*
 * 
 * Todo el objetivo 3 de ofrecer sincronizacion basado en mutex
 *
 */
//Crear un mutex
int sis_crear_mutex(){
        //Declaramos las variables de nombres y tipos especificados
        char *nombre;
	int tipo;
        
	//Para cada uno de ellos le asginamos en un hueco del registro
	nombre=(char *)leer_registro(1);
	tipo=(int)leer_registro(2);
	
	BCP * p_proc_anterior;
 	int pos;
 	int existe; 

 	//Comprueba si en posicion hay descriptor
 	pos = tiene_descriptor();
 	//En caso de no encontrarlo devuelve un -1
 	if(pos == -1){
 		printk("ERROR, no tiene descriptor. \n");
 		return -1;
 	}

 	//Si encuentra un descriptor es que existe un mutex y devuelve un error
 	existe = existe_mutex(nombre);
 	if(existe) {
 		printk("ERROR, ya existe un mutex con este nombre. \n");
 		return -1;
 	}

 	//Comprueba si hay disponibilidad
 	int disponibilidad = libre();

 	//Mientras disponibilidad este llena se ejecuta el bucle
 	while(disponibilidad == LLENO){
 		p_proc_actual->estado = BLOQUEADO;
 		p_proc_actual->replanificacion = 0;
 		nivel_anterior = fijar_nivel_int(NIVEL_3);
 		eliminar_primero(&lista_listos);
 		//Insertamos al final de la lista mutex el proceso actual
 		insertar_ultimo(&lista_de_mutex, p_proc_actual);
 		//Cambio de contexto del proceso anterior al actual
 		p_proc_anterior = p_proc_actual;
 		//Esperamos a que haya un proceso listo
 		p_proc_actual = planificador();
 		//Imprimimos por pantalla un mensaje
 		printk("Crea mutex del proceso %d a %d\n", p_proc_anterior->id, p_proc_actual->id);
 		//Restauramos contexto del nuevo proceso actual
 		cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
 		fijar_nivel_int(nivel_anterior);
 		disponibilidad = libre();
 	}

 	//Despues de comprobar todo creamos el mutex segun la estructuta de la cabecera
 	array_mutex[disponibilidad].procesos_mutex++;
 	array_mutex[disponibilidad].nombre = strdup(nombre);
	array_mutex[disponibilidad].tipo = tipo;
 	array_mutex[disponibilidad].propietario = p_proc_actual->id;
 	
 	

 	p_proc_actual->descriptores[pos].descript = disponibilidad;
 	p_proc_actual->descriptores[pos].libre = 1;
	
	printk("Mutex tipo %d creado \n", tipo);
	
 	return disponibilidad;
	             
}
//Abre un mutex
int sis_abrir_mutex(){
        //Declaramos la variable nombre 
        char *nombre;
        
	//Guardamos el nombre en el registro 1
	nombre=(char *)leer_registro(1);
	
	/*
	
	//Abrimos el muetx
        //Espacio para crear mutex
	int pMutex = -1;
	for(int i=0; i<NUM_MUT; i++){
	    array_mutex[i].procesos[p_proc_actual->id] = 1;
	    pMutex = i;
	}
	
	//Busca el mutex por su descriptor
	int descrip = -1;
	//Bucle donde recorre numero maximo de mutex que puede tener abiertos un proceso
	for(int i=0; i<NUM_MUT_PROC; i++){
	  if(p_proc_actual->array_mutex_proceso[i] == NULL){
	       p_proc_actual->array_mutex_proceso[i] = &array_mutex[pMutex];
	       p_proc_actual->numeroMutex++;
	       descrip = i;
	  }
	}
	
	*/
	
 	int pos;
 	int existe;
 	int descriptor;

 	pos = tiene_descriptor();
 	//Si no hay descriptor:
 	if(pos == -1){
 		printk("ERROR, no hay descriptores libres para el proceso %d\n", p_proc_actual->id);
		
 		return -1;
 	}

 	//Si hay descriptor pero no exite nombre, otro mensaje de error
 	existe = existe_mutex(nombre);
 	if(!existe){
 		printk("Error debido a que no existe el mutex con ese nombre\n");
		
 		return -1;
 	}

 	//Si hemos llegado hasta aqui se han cumplido las precondiciones por lo que concedemos el descriptor al mutex
 	descriptor = pos_mutex(nombre);
 	array_mutex[descriptor].procesos_mutex++;
 	p_proc_actual->descriptores[pos].descript = descriptor;
 	p_proc_actual->descriptores[pos].libre = 1;

        printk("El mutex %d abierto.\n",descriptor);	
	
 	return descriptor;
      
}
//Intenta bloquear el mutex, no recursivo
int sis_lock(){
        unsigned int mutexid;
	mutexid=(unsigned int)leer_registro(1);
	
	BCP*p_proc_anterior;
	int bloqueado;

	mutex mut = array_mutex[mutexid];
	
	do{
		bloqueado = 0;
		if(mut.procesos_mutex > 0){
			//Verificamos si esta o no bloqueado
			if(mut.bloqueado > 0){
				//Comprobamos si es RECURSIVO
				if(mut.tipo == RECURSIVO){
					//Comprobamos si es el propietario del mutex
					if(mut.propietario == p_proc_actual->id){
						//Aumentamos el numero de bloqueos en el mutex
						mut.bloqueado++;
					}else{
				        //En caso contrario bloquea al proceso
						p_proc_actual->estado = BLOQUEADO;
						//No hace ya cambios de contexto involuntario
						p_proc_actual->replanificacion = 0;
						nivel_anterior = fijar_nivel_int(NIVEL_3);

						eliminar_primero(&lista_listos);
						//Lo insertamos en la lista de bloqueados por un lock
						insertar_ultimo(&lista_de_bloqueados, p_proc_actual);
						//Hacemos un cambio de Contexto
						p_proc_anterior = p_proc_actual;
						p_proc_actual = planificador();

						printk("Del proceso anterior %d a actual %d por un Lock. \n", p_proc_anterior->id, p_proc_actual->id);
						
						//Restauramos el contexto del nuevo actual
						cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
						fijar_nivel_int(nivel_anterior);
						//Y bloqueamos para que no se escape ningun proceso
						bloqueado = 1;
					}
				}else if(mut.tipo == NO_RECURSIVO){
					//Vemos si es el propietario del mutex
					if(mut.propietario == p_proc_actual->id){
						//Comprobamos si ya hay un proceso bloqueandolo
						//Imprimimos por pantalla error por interbloqueo
						printk("Error debido a un interbloqueo.\n");
							
						//En caso contrario bloqueamos el mutex
						mut.bloqueado++;
						
						return -1;
					}else{
				        //Bloqueamos si no es propietario
						p_proc_actual->estado = BLOQUEADO;
						//Ya no es necesario hacer cambio de contexto involuntario
						p_proc_actual->replanificacion = 0;
						nivel_anterior = fijar_nivel_int(NIVEL_3);
						
						eliminar_primero(&lista_listos);
						insertar_ultimo(&lista_de_bloqueados, p_proc_actual);
						//Cambio de contexto
						p_proc_anterior = p_proc_actual;
						p_proc_actual = planificador();

						printk("Del proceso anterior %d a actual %d por un Lock. \n", p_proc_anterior->id, p_proc_actual->id);
						
						//Restauramos el contexto del nuevo actual
						cambio_contexto(&(p_proc_anterior->contexto_regs),&(p_proc_actual->contexto_regs));
						fijar_nivel_int(nivel_anterior);

						//Y bloqueamos para que no se escape ningun proceso
						bloqueado = 1;
					}
				}
			}else if(mut.bloqueado == 0){
				//Hacemos que el proceso actual pase a ser el nuevo propietario y bloqueamos al mutex
			        mut.propietario = p_proc_actual->id;	
			        mut.bloqueado++;
			}else{
				printk("Error del propio mutex. \n");
				
				return -1;
			}
		}else{
			printk("ERROR: por hacer lock a un mutex no iniciado \n");
			
			return -1;
		}
	}while(bloqueado);
	
	return 0;
}
//Desbloquear el mutex, recursivo
int sis_unlock(){
        unsigned int mutexid;
	mutexid=(unsigned int)leer_registro(1);
	
	//Desbloquear el mutex
	BCP*pr_bloqueado;
	
	mutex mut = array_mutex[mutexid];

	//verificamos que existe el mutex
	if(mut.procesos_mutex > 0){
		//Comprobamos si esta bloqueado
		if(mut.bloqueado > 0){
			//Comprobamos si es recursivo o no
			if(mut.tipo == RECURSIVO){
				//Comprobamos si es el propietario del bloqueo
				if(mut.propietario == p_proc_actual->id){
					//Disminuimos el numero de bloqueos
					mut.bloqueado--;
					if(mut.bloqueado == 0){
						pr_bloqueado = lista_de_bloqueados.primero;
						//Verificamos si hay algun proc en espera
						if(pr_bloqueado != NULL){
							//El proceso bloqueado lo ponemos en Listo despertandolo
							pr_bloqueado->estado = LISTO;
							nivel_anterior = fijar_nivel_int(NIVEL_3);
							eliminar_primero(&lista_de_bloqueados);
							insertar_ultimo(&lista_listos, pr_bloqueado);
							fijar_nivel_int(nivel_anterior);
						}
					}
				}else{
					printk("ERROR: El mutex no tiene el mismo propietario que el proceso actual. \n");
				}
			}else if(mut.tipo == NO_RECURSIVO){
				//Verificamos si es el propietario
				if(mut.propietario == p_proc_actual->id){
					//Desbloqueamos
					mut.bloqueado--;
					if(mut.bloqueado != 0){
						printk("ERROR: No se ha desbloqueado el mutex no recursivo. \n");
						
						return -1;
					}
					pr_bloqueado = lista_de_bloqueados.primero;
					//Verificamos si hay algun proceso en espera
					if(pr_bloqueado != NULL){
						pr_bloqueado->estado = LISTO;
						nivel_anterior = fijar_nivel_int(NIVEL_3);
						eliminar_primero(&lista_de_bloqueados);
						insertar_ultimo(&lista_listos, pr_bloqueado);
						fijar_nivel_int(nivel_anterior);
					}
				}else{
					printk("ERROR: El mutex no tiene el mismo propietario que el proceso actual. \n");
				}
			}
		}
		//En caso de no estar bloqueado
		else if(mut.bloqueado == 0){
			printk("ERROR: Un mutex que no esta bloqueado no puede ser desbloqueado. \n");
		}else{
			printk("Otro error del mutex. \n");
			
			return -1;
		}
	}
	else{
		printk("ERROR: por hacer unlock a un mutex no abierto \n");
		
		return -1;
	}
	
	return 0;
}
//Para cerrar el mutex
int sis_cerrar_mutex(){
        unsigned int mutexid;
	mutexid=(unsigned int)leer_registro(1);
	
	BCP * pr_bloqueado_mutex;
	BCP * pr_bloqueado;
	
	//Comprobamos si existe el descriptor que se quiere cerrar
	int existe = tiene_descriptor(mutexid);
	if(existe < 0){
		printk("No existe el mutex con el descriptor dado. \n");	
		return -1;
	}

	p_proc_actual->descriptores[existe].libre = 0;
	if(array_mutex[mutexid].procesos_mutex <= 0){
		printk("No existe el mutex que se quiere cerrar. \n");		
		return -1;
	}
	
	array_mutex[mutexid].procesos_mutex--;	
	//Hay mutex disponible en caso de llegar a 0
	if(array_mutex[mutexid].propietario == p_proc_actual->id){
		array_mutex[mutexid].bloqueado = 0;
		pr_bloqueado = lista_de_bloqueados.primero;
		if(pr_bloqueado != NULL){
		        //Cambio de contexto
			pr_bloqueado->estado = LISTO;
			nivel_anterior = fijar_nivel_int(NIVEL_3);
			eliminar_primero(&lista_de_bloqueados);
			insertar_ultimo(&lista_listos,pr_bloqueado);
			fijar_nivel_int(nivel_anterior);
			printk("Mutex cerrado. \n");
		}
	}
	
	//En caso de el proceso mutex sea 0 y y no este bloqueado cerramos el mutex
	if(array_mutex[mutexid].procesos_mutex == 0){
		pr_bloqueado_mutex = lista_de_mutex.primero;
		//Verificamos si hay algun proceso esperando
		if(pr_bloqueado_mutex != NULL){
			pr_bloqueado_mutex->estado = LISTO;
			nivel_anterior = fijar_nivel_int(NIVEL_3);
			eliminar_primero(&lista_de_mutex);
			insertar_ultimo(&lista_listos, pr_bloqueado_mutex);
			fijar_nivel_int(nivel_anterior);
			printk("Mutex cerrado. \n");
		}
	}
	
	return 0;
}


//Objetivo parcial 5
//input, manejo basico de la entrada por teclado
int leer_caracter(){
	while(1){		      
		 if(BufferChar != 0){
		        int i;
		        int nivel_int = fijar_nivel_int(NIVEL_2);

		        // Saca el primer caracter
		        char car = Buffer[0];
				//quita una posicion en el buffer
		        BufferChar--;

		        // Coloca el buffer en orden
		        for (i = 0; i < BufferChar; i++){
		              Buffer[i] = Buffer[i + 1];
		        }

		        fijar_nivel_int(nivel_int);

	                return (long)car;

		 }else{
			// Si el buffer no tiene nada lo bloqueamos
			p_proc_actual->estado = BLOQUEADO;
			p_proc_actual->blocLectura = 1;
				//ponemos nivel de bloqueo
			int nivel_int = fijar_nivel_int(NIVEL_3);
			//eliminamos el proceso actualde la lista de listos
			eliminar_elem(&lista_listos, p_proc_actual);
			insertar_ultimo(&lista_dormidos, p_proc_actual);
			fijar_nivel_int(nivel_int);
			nivel_int = fijar_nivel_int(NIVEL_2);
			//bloqua el proceso actual
			BCP *proceso_bloqueado = p_proc_actual;
			//llama al planificador
			p_proc_actual = planificador();
			//cambia de contexto
			cambio_contexto(&(proceso_bloqueado->contexto_regs), &(p_proc_actual->contexto_regs));
			fijar_nivel_int(nivel_int);
	         }
	}
}



/*
 *
 * Rutina de inicialización invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

        iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */
		//inicia las excepciones
	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */
	

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
