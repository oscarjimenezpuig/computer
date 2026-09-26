// ENSAMBLADOR: Proporciona un lenguaje ensamblador para computer

#include <stdio.h>

// CONSTANTES

//Constantes copiadas de computer

#define SCRW 144 //numero de columnas
#define SCRH 160 //numero de filas
#define PPB 4 //pixeles por byte de memoria

#define IZP 0 //inicio de la pagina cero
#define DZP 1 //dimension de la pagina cero
#define IRG (IZP+DZP) //inicio de los registros
#define DRG 9 //dimension de los registros
#define IST (IRG+DRG) //inicio de la pila
#define DST 256 //dimension del stack
#define IPR (IST+DST) //inicio del programa
#define DPR 4096 //dimension del programa
#define IRM (IPR+DPR) //inicio de la ram de variables
#define DRM 2048 //dimension de la ram de variables
#define IVR (IRM+DRM) //inicio de la memoria visual
#define DVR ((SCRW*SCRH)/PPB) //dimension de la memoria visual
#define IIN (IVR+DVR) //inicio de la entrada del teclado
#define DIN 1 //dimension de la entrada del teclado
#define IWC (IIN+DIN) //inicio del reloj
#define DWC 4 //final del reloj

#define DMEM (DZP+DRG+DST+DPR+DRM+DVR+DIN+DWC) //memoria total

#define RA IRG //registro A
#define RB (RA+1) //registro B
#define RX (RB+1) //regitro X (para bucles...)
#define RPC (RX+2) //registro de linea de programa
#define RHP (RPC+2) //registro de la linea de la pila
#define RF  (RHP+2) //registro de la bandera

#define FC 1 //flag carry
#define FZ 2 //flag cero
#define FN 4 //flag negativo
#define FWAI 8 //flag de wait until refresh
#define FJD 16 //flag de salto hecho

#define KUP 1 //bandera de tecla arriba
#define KRG 2 //bandera de tecla derecha
#define KLF 4 //bandera de tecla izquierda
#define KDW 8 //bandera de tecla abajo
#define KUA 16 //bandera de tecla de uso A
#define KUB 32 //bandera de tecla de uso B
#define KPA 64 //bandera de pausa
#define KQT 128 //bandera de quit

#define HALT 0 //para el sistema->0
#define WAIT 1 //interrupcion hasta refresco->0
#define LDAX 2 //copia el valor de la direccion apuntada por X en A->0
#define STAX 3 //copia el valor de A en la direccion apuntada por X->0
#define INCX 4 //incrementa el valor de la direccion en 1->0
#define DECX 5 //decrementa la direccion de X en 1->0
#define NOTA 6 //niega el valor A->0
#define SHLA 7 //desplaza de los bits A izquierda (carry y zero)->0
#define SHRA 8 //desplaza de los bits A derecha (carry y zero)->0
#define ROLA 9 //rota izquierda bits de A (carry y zero)->0
#define RORA 10 //rota derecha bits de A (carry y zero)->0
#define PSHA 11 //valor de A a la pila->0
#define POPA 12 //valor de la pila a A->0
#define RET 13 //se saca la direccion y se va alli->0
#define SWAB 14 //intercambia los valores de RA i RB->
#define LDIA 20 //valor directo a A->1
#define CPIA 21 //compara el valor directo con A (A-val simulada) activa flag cero si son iguales o flag negativo si el dato entrado es menor que A->1
#define LDAd 30 //copia de dir a A->2
#define STAd 31 //copia de A a dir->2
#define LDXd 32 //guarda la direccion d en X->2
#define ADDd 33 //suma A con valor en d y deposita en A (carry y zero)->2
#define SUBd 34 //resta A con valor en d (carry y zero)->2
#define ANDd 35 //and A con valor en d (zero)->2
#define ORd 36 //or A con valor en d (zero)->2
#define XORd 37 //xor A con valor en d (zero)->2
#define JMPd 38 //salta a la direccion d->2
#define JFCd 39 //si flag carry salta a d->2
#define JNCd 40 //si no carry salta a d->2
#define JFZd 41 //si flag zero salta a d->2
#define JNZd 42 //si no flag zero salta a d->2
#define JFNd 43 //si flag negativo salta a d->2
#define JNNd 44 //si flag no negativo salta a d->2
#define CLLd 45 //va a la direccion y actual se guarda en la pila->2

// Constantes propias de ensamblador

#define LOADS "LOD"
#define STATS "STA"
#define INCS "INC"
#define DECS "DEC"
#define NOTS "NOT"
#define SHLS "SHL"
#define SHRS "SHR"
#define ROLS "ROL"
#define RORS "ROR"
#define PSHS "PSH"
#define POPS "POP"
#define RETS "RET"
#define SWAS "SWA"
#define CMPS "CMP"
#define ADDS "ADD"
#define SUBS "SUB"
#define ANDS "AND"
#define ORS "OR"
#define XORS "XOR"
#define JMS "JM"
#define JMNS "JMN"
#define CLLS "CLL"

//Estructura de la linea:
//  Cinco valores que representan la direccion relativa del programa (no leidos)
//  Orden
//  Primer complemento A o X o bandera (Z,N,C)
//  Segundo complemento valor o  [direccion]

