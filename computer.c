#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "computer.h"

#define PIXDIM 4 //dimension del pixel

#define FION(F) ((((F) & memory[RF])!=0)?1:0) //comprobacion de flag
#define KION(K) ((((K) & memory[IIN])!=0)?1:0) //comprobacion de tecla

#define FON(F) (memory[RF]|=(F)) //conecta flag
#define FOFF(F) (memory[RF]&=(~(F))) //desconecta flag
#define KON(K) (memory[IIN]|=(K)) //conecta tecla

#define MSET(D,V) (memory[(D)]=V) //asigna valor a una direccion
#define MGET(D) memory[D] //consigue el valor de una direccion

#define TOD(D,U) ((D)+(U)*256) //se pasan dos bytes a direccion

static Display* display=NULL;
static Colormap colormap;
static Window window;
static Pixmap virtual;
static GC graphic;
static int min_key_code=0;
static int max_key_code=0;
static memory_t memory;
static unsigned long color[4];

static int err_prt(char* s,int e) {
    //impresion de un error con codigo e
    fprintf(stderr,"ERROR: %s\n",s);
    return e;
}

static void wtc_inc() {
    //se incrementa en 1 el valor del reloj
    byte_t* p=memory+IWC;
    while(p!=memory+IWC+DWC) {
        *p=*p+1;
        if(*p==0) p++;
        else break;
    }
}

static void wtc_zer() {
    //pone a cero el reloj
    byte_t* p=memory+IWC;
    while(p!=memory+IWC+DWC) *p++=0;
}

static unsigned int wtc_to_int() {
    unsigned long int time=0;
    byte_t* p=memory+IWC;
    unsigned long int factor=1;
    while(p!=memory+IWC+DWC) {
        time+=(*p)*factor;
        factor*=256;
        p++;
    }
    return time;
}

static unsigned long col_new(byte_t brg) {
	XColor xc;
	xc.flags=DoRed|DoGreen|DoBlue;
	xc.red=xc.green=xc.blue=21675*brg;
	XAllocColor(display,colormap,&xc);
	return xc.pixel;
}


static void sqr_drw(int x,int y,byte_t c) {
    //dibuja un cuadrado de dimension d de color c
	XSetForeground(display,graphic,color[c]);
	XFillRectangle(display,virtual,graphic,x,y,PIXDIM,PIXDIM);
}

static void scr_drw() {
    //dibuja la pantalla cada cierto tiempo
    const unsigned short SCR_W=SCRW*PIXDIM;
    const unsigned short SCR_H=SCRH*PIXDIM;
    const unsigned long int TIME=VMH*TMH;
    if(wtc_to_int()>=TIME) {
        wtc_zer();
        memory[RF]&=(~FWAI);
        byte_t* p=memory+IVR;
        byte_t msc=3;
        for(int f=0;f<SCRH;f++) {
            for(int c=0;c<SCRW;c++) {
                byte_t col=(*p & msc);
                sqr_drw(c*PIXDIM,f*PIXDIM,col);
                if(msc==192) {
                    msc=3;
                    p++;
                } else {
                    msc*=4;
                }
            }
        }
        XCopyArea(display,virtual,window,graphic,0,0,SCR_W,SCR_H,0,0);
	    while(XPending(display)==0);
	    XFlush(display);
    } else {
        wtc_inc();
    }
}

static void scr_lis() {
    //funcion que se encarga del registro de teclas y guardarlas en memoria
    const char* KEYS="iljkzxpq"; //teclas utilizadas
    XEvent ev;
    KeySym ks;
    int ty=0;
    while(XPending(display)>0) {
        XNextEvent(display,&ev);
        ty=(ev.type==KeyPress)?1:(ev.type==KeyRelease)?-1:0;
        if(ty) {
            ks=XLookupKeysym(&ev.xkey,0);
            char k='a'+(ks-XK_a);
            const char* pk=KEYS;
            byte_t msk=1;
            while(*pk!='\0') {
                if(*pk==k) {
                    if(ty==1) memory[IIN]|=msk;
                    else  memory[IIN]&=(~msk);
                    break;
                }
                pk++;
                msk=msk<<1;
            }

        }
    }
} 

static int prg_inp(char* program) {
    //lee el programa y devuelve el codigo de error si lo hubiera
    char* ptr=program;
    unsigned char fac=100;
    byte_t byte=0;
    byte_t* pp=memory+IPR;
    unsigned short size=0;
    while(*ptr!='\0') {
        if(size<DPR) {
            byte+=(*ptr-'0')*fac;
            fac=fac/10;
            if(!fac) {
                *pp++=byte;
                byte=0;
                size++;
                fac=100;
            }
            ptr++;
        } else return err_prt("Program is too long",-1);
    }
    return 0;
}

static void dir_inc(byte_t* d,byte_t* u) {
    //incrementa en 1 la direccion 
    if((*d)==255) {
        *u+=1;
        *d=0;
    } else {
        (*d)+=1;
    }
}

static void dir_dec(byte_t* d,byte_t* u) {
    //decrementa en uno la direccion
    if(*d==0) {
        *u-=1;
        *d+=255;
    } else *d-=1;
}

static byte_t* dir_get(byte_t d,byte_t u) {
    //obtiene el puntero de una direccion
    unsigned short dir=TOD(u,d);
    if(dir<DMEM) return memory+dir;
    return NULL;
}

static int dir_tud(byte_t* ptr,byte_t* d,byte_t* u) {
    //pasa puntero a dos datos de direccion
    if(ptr>=memory && ptr<memory+DMEM) {
        unsigned short dir=ptr-memory;
        *u=dir%256;
        *d=dir/256;
        return 1;
    }
    return 0;
}  

static void stk_psh(byte_t v) {
    //introduce byte en el stack y desplaza el indicador de la pila
    memory[TOD(memory[RHP],memory[RHP+1])]=v;
    dir_inc(memory+RHP,memory+RHP+1);
}

static byte_t stk_pop() {
    //disminuye en 1 la direccion y devuelve el valor (siempre que no sea l principio de la pila)
    byte_t ret=0;
    unsigned short dira=TOD(memory[RHP],memory[RHP+1]);
    if(dira>IST) {
        ret=memory[dira-1];
        dir_dec(memory+RHP,memory+RHP+1);
    }
    return ret;
}

static byte_t des_l(byte_t oc,byte ra) {
    if(ra & 128) FON(FC);
    ra=ra<<1;
    if(FION(FC) && oc==ROLA) ra|=1;
    return ra;
}

static byte_t des_r(byte_t oc,byte ra) {
    if(ra & 1) FON(FC);
    ra=ra>>1;
    if(FION(FC) && oc==RORA) ra|=128;
    return ra;
}

static void ula(byte_t oc) {
    //calculos a partir de la entrada del opcode
    //todos los flags de calculo eliminados
    byte_t ra=memory[RA];
    FOFF(FC|FZ|FN);
    if(oc<20) {
        switch(oc) {
            case NOTA:
                ra=~ra;
                break;
            case SHLA:
            case ROLA:
                ra=des_l(oc,ra);
                break;
            case SHRA:
            case RORA:
                ra=des_r(oc,ra);
                break;
        }
    } else if(oc<30) {
        switch(oc) {
            case CPIA:
                //TODO Continuar programacion de ULA
                //Hay que poner flag cero si ra es 0







static int prg_exe() {
    //ejecucion del programa
    int err=0;
    byte_t opcode=rpc_giv();
    if(opcode==LDIA || opcode==CPIA) one_byte(opcode);
    else if(opcode==HALT || opcode==WAIT || (opcode>=LDAX && opcode<=DECX) || (opcode>=NOTA && opcode<=RORA) || opcode==PSHA || opcode==POPA || opcode==RET) zero_byte(opcode);
    else (err=two_byte(opcode));
    if(!err) { 
        if(FION(FJD)) FOFF(FJD);
        else (err=rpc_inc());
    }
    return err;
}


void cmp_ini() {
    const unsigned short SCR_W=SCRW*PIXDIM;
    const unsigned short SCR_H=SCRH*PIXDIM;
    byte_t* p=memory;
    while(p!=memory+DMEM) *p++=0;
    memory[RPC]=(IPR%256);
    memory[RPC+1]=(IPR/256);
    memory[RHP]=(IST%256);
    memory[RHP+1]=(IST/256);
    mem_prt();//dbg
    for(unsigned short dir=IWC;dir<IWC+DWC;dir++) memory[dir]=255;
    int screenum=0;
	display=XOpenDisplay(0);
	if(display) {
		int screennum=XDefaultScreen(display);
		colormap=XDefaultColormap(display,screenum);
        for(byte_t k=0;k<4;k++) color[3-k]=col_new(k);
		window=XCreateSimpleWindow(display,RootWindow(display,screennum),0,0,SCR_W,SCR_H,0,0,0);
		XWindowAttributes xwa;
		XGetWindowAttributes(display,window,&xwa);
		virtual=XCreatePixmap(display,window,SCR_W,SCR_H,xwa.depth);
		static XGCValues gv;
		gv.line_width=1;
		gv.foreground=color[0];
		gv.background=color[3];
		graphic=XCreateGC(display,window,GCForeground|GCBackground|GCLineWidth,&gv);
		XSelectInput(display,window,StructureNotifyMask|KeyPressMask|KeyReleaseMask);
		XMapWindow(display,window);
		XEvent event;
		int mapped=0;
		while(XEventsQueued(display,QueuedAlready) || !mapped) {
			XNextEvent(display,&event);
			mapped=(!mapped)?(event.type==MapNotify):1;
		}
		XDisplayKeycodes(display,&min_key_code,&max_key_code);
		scr_drw();
    }
}

void cmp_end() {
    XUnmapWindow(display,window);
	XDestroyWindow(display,window);
	XFreePixmap(display,virtual);
	XFreeColormap(display,colormap);
	XFreeGC(display,graphic);
	XEvent e;
	while(XEventsQueued(display,QueuedAlready)) {
			XNextEvent(display,&e);
	}
	XCloseDisplay(display);
}

static void sec_prt(unsigned short dir,unsigned short length) {
    printf("Inicio=%i Final=%i\n",dir,dir+length-1);
    unsigned char counter=0;
    for(unsigned short p=dir;p<dir+length;p++) {
        if(counter==10) {
            counter=0;
            puts("");
        } else counter++;
        printf("%03i ",memory[p]);
    }
    puts("");
}

void mem_prt() {
    puts("REGISTROS");
    sec_prt(IRG,DRG);
    puts("PILA");
    sec_prt(IST,DST);
    /*
    puts("PROGRAMA");
    sec_prt(IPR,DPR);
    puts("RAM");
    sec_prt(IRM,DRM);
    puts("PANTALLA");
    sec_prt(IVR,DVR);
    puts("TECLADO");
    sec_prt(IIN,DIN);
    puts("RELOJ");
    sec_prt(IWC,DWC);
    */
}

int main(int program_len,char* program[]) {
    int err=0;
    //se introduce el programa como cadena de caracteres de longitud byte toda seguida
    cmp_ini();
    if(program_len>0 && !(err=prg_inp(program[1]))) {
        while(!KION(KQT) && !err) {
            scr_drw();
            if(!FION(FWAI)) {
                scr_lis();
                if(!KION(KPA) && !KION(KQT)) {
                    err=prg_exe();
                }
            }
        }
    }
    mem_prt();//dbg
    cmp_end();
    return err;
}
