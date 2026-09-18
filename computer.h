// COMPUTER:  Simula computadora grafica de 8 bits con bus de datos de 16 bits

// CONSTANTES

#define SCRW 40 //numero de columnas
#define SCRH 40 //numero de filas
#define PPB 4 //pixeles por byte de memoria

#define IZP 0 //inicio de la pagina cero
#define DZP 1 //dimension de la pagina cero
#define IRG (IZP+DZP) //inicio de los registros
#define DRG 7 //dimension de los registros
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

#define DMEM (DZP+DRG+DST+DPR+DRM+DVR+DIN+DWC)

#define RA IRG //registro A
#define RB (RA+1) //registro B
#define RX (RB+1) //regitro X (para bucles...)
#define RPC (RX+1) //registro de linea de programa
#define RHP (RPC+2) //registro de la linea de la pila
#define RF  (RHP+2) //registro de la bandera

#define FC 1 //flag carry
#define FZ 2 //flag cero
#define FN 4 //flag negativo
#define FWAI 8 //flag de wait until refresh

#define KUP 1 //bandera de tecla arriba
#define KRG 2 //bandera de tecla derecha
#define KLF 4 //bandera de tecla izquierda
#define KDW 8 //bandera de tecla abajo
#define KUA 16 //bandera de tecla de uso A
#define KUB 32 //bandera de tecla de uso B
#define KPA 64 //bandera de pausa
#define KQT 128 //bandera de quit

#define VMH 1.0 //velocidad en megahercios
#define TMH 16666.6 //numero de instrucciones por megaherzio

// TIPOS

typedef unsigned char byte_t;

typedef byte_t memory_t[DMEM];

// VARIABLES


// FUNCIONES

void cmp_ini();
//se inicia la memoria

void cmp_end();
//se libera todo el espacio (xlib)

void mem_prt();
//se imprimen todos los bytes de la memoria

