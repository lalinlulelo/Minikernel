/*
 *  usuario/include/servicios.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 *  Autores: Zhong Hao Lin Chen, Alvaro Barroso Mato
 */

/*
 *
 * Fichero de cabecera que contiene los prototipos de funciones de
 * biblioteca que proporcionan la interfaz de llamadas al sistema.
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef SERVICIOS_H
#define SERVICIOS_H

/* Evita el uso del printf de la bilioteca estándar */
#define printf escribirf

//Objetivo parcial 3, definir el tipo de mutex
#define NO_RECURSIVO 0
#define RECURSIVO 1

/* Funcion de biblioteca */
int escribirf(const char *formato, ...);

/* Llamadas al sistema proporcionadas */
int crear_proceso(char *prog);
int terminar_proceso();
int escribir(char *texto, unsigned int longi);
//Objetivo parcial 1
int obtener_id_pr(); //prototipo funcion de interfaz
//Objetivo  parcial 2
int dormir(unsigned int segundos);
//Objetivo parcial 3, interfaz de los servicios de mutex
int crear_mutex(char *nombre, int tipo);
int abrir_mutex(char *nombre);
int lock(unsigned int mutexid);
int unlock(unsigned int mutexid);
int cerrar_mutex(unsigned int mutexid);
//Objetivo parcial 5
int leer_caracter();

#endif /* SERVICIOS_H */

