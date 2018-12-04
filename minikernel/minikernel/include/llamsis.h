/*
 *  minikernel/kernel/include/llamsis.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 * 
 *  Autores: Zhong Hao Lin Chen, Alvaro Barroso Mato
 */

/*
 *
 * Fichero de cabecera que contiene el numero asociado a cada llamada
 *
 * 	SE DEBE MODIFICAR PARA INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef _LLAMSIS_H
#define _LLAMSIS_H

/* Numero de llamadas disponibles */
#define NSERVICIOS 11

#define CREAR_PROCESO 0
#define TERMINAR_PROCESO 1
#define ESCRIBIR 2
//incrementar numero de llamadas y asignar valor m'as alto
#define OBTENER_ID_PR 3 //Objetivo parcial 1
#define DORMIR 4     //Objetivo parcial 2
//Objetivos parciales 3, Todas las llamadas asociadas al mutex
#define CREAR_MUTEX 5   
#define ABRIR_MUTEX 6
#define LOCK 7
#define UNLOCK 8
#define CERRAR_MUTEX 9
//Objetivo parcial 5, llamada a entrada por teclado
#define LEER_CARACTER 10

#endif /* _LLAMSIS_H */

