// COMPUTER:  Simula computadora grafica de 8 bits con bus de datos de 16 bits

// CONSTANTES

#define SCRW 144 //numero de columnas
#define SCRH 160 //numero de filas
#define PPB 4 //pixeles por byte de memoria

#define IZP 0 //inicio de la pagina cero
#define DZP 1 //dimension de la pagina cero
#define IRG (IZP+DZP) //inicio de los registros
#define DRG 8 //dimension de los registros
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

#define IRWM IRM //inicio de la memoria que se puede leer y escribir
#define DRWM (DRM+DVR) //dimension de la memoria que se puede leer escribir
#define IORM IIN //inicio de la memoria solo leida
#define DORM (DIN+DWC) //dimension de la memoria solo leida

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

#define HALT 0 //para el sistema
#define WAIT 1 //interrupcion hasta refresco
#define LDAd 2 //copia de dir a A
#define STAd 3 //copia de A a dir
#define LDIA 4 //valor directo a A
#define CPIA 5 //compara el valor directo con A (A-val simulada) activa flags
#define LDXd 6 //guarda la direccion d en X
#define LDAX 7 //copia el valor de la direccion apuntada por X en A
#define STAX 8 //copia el valor de A en la direccion apuntada por X
#define INCX 9 //incrementa el valor de la direccion en 1
#define DECX 10 //decrementa la direccion de X en 1
#define ADDd 11 //suma A con valor en d y deposita en A (carry y zero)
#define SUBd 12 //resta A con valor en d (carry y zero)
#define ANDd 13 //and A con valor en d (zero)
#define ORd 14 //or A con valor en d (zero)
#define XORd 15 //xor A con valor en d (zero)
#define SHLA 16 //desplaza de los bits A izquierda (carry y zero)
#define SHRA 17 //desplaza de los bits A derecha (carry y zero)
#define ROLA 18 //rota izquierda bits de A (carry y zero)
#define RORA 19 //rota derecha bits de A (carry y zero)
#define JMPd 20 //salta a la direccion d
#define JFCd 21 //si flag carry salta a d
#define JNCd 22 //si no carry salta a d
#define JFZd 23 //si flag zero salta a d
#define JNZd 24 //si no flag zero salta a d
#define PSHA 25 //valor de A a la pila
#define POPA 26 //valor de la pila a A
#define CLLd 27 //va a la direccion y actual se guarda en la pila
#define RET 28 //se saca la direccion y se va alli


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

