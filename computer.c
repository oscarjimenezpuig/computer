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
    fprintf(stderr,"ERROR: %s",s);
    return e;
}

static unsigned long col_new(byte_t brg) {
	XColor xc;
	xc.flags=DoRed|DoGreen|DoBlue;
	xc.red=xc.green=xc.blue=21675*brg;
	XAllocColor(display,colormap,&xc);
	return xc.pixel;
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

unsigned int wtc_to_int() {
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
    if(*d==0) {
        *u-=1;
        *d+=255;
    } else *d-=1;
}

static int rpc_inc() {
    dir_inc(memory+RPC,memory+RPC+1);
    if(TOD(memory[RPC],memory[RPC+1])<IPR+DPR) return 0;
    else return err_prt("HALT Opcode not found",-3);
}

static unsigned short rpc_giv() {
    //da la direccion de ejecucion como un short
    return TOD(memory[RPC],memory[RPC+1]);
}

static void flg_zer() {
    //comprueba que hay flag zero
    if(memory[RA]==0) FON(FZ);
    else FOFF(FZ);
}

#define COF(O,F) (o==(O) && FION((F)))
#define CON(O,F) (o==(O) && !FION((F)))

static void jmp_cas(byte_t o,byte_t d,byte_t u) {
    if(o==JMPd || COF(JFCd,FC) || COF(JFZd,FZ) || COF(JFNd,FN) || CON(JNCd,FC) || CON(JNZd,FZ) || CON(JNNd,FN)) {
        memory[RPC]=d;
        memory[RPC+1]=u;
        FON(FJD);
    }
}

#undef COF
#undef CON

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

static void zero_byte(byte_t opcode) {
}

static void one_byte(byte_t opcode) {
    rpc_inc();
    byte_t d=memory[rpc_giv()];
    if(opcode==LDIA) {
        memory[RA]=d;
    } else if(opcode==CPIA) {
        byte_t da=memory[RA];
        FOFF(FZ|FN);
        if(da==d) FON(FZ);
        else if(d<da) FON(FN);
    }
}


static int two_byte(byte_t opcode) {
    //ejecuta las ordenes que necesitan tres bytes de entrada
    byte_t b[2];
    for(byte_t k=0;k<3;k++) {
        rpc_inc();
        b[k]=memory[rpc_giv()];
    }
    if(opcode==LDAd) {
        memory[RA]=memory[TOD(b[0],b[1])];
    } else if(opcode==STAd) {
        memory[TOD(b[0],b[1])]=memory[RA];
    } else if(opcode==LDXd) {
        memory[RX]=b[0];
        memory[RX+1]=b[1];
    } else if(opcode==ADDd || opcode==SUBd) {
        byte_t val=memory[TOD(b[0],b[1])];
        if(opcode==SUBd) FOFF(FN);
        else FOFF(FC);
        while(val--) {
            memory[RA]+=(opcode==ADDd)?1:-1;
            if(memory[RA]==0 && opcode==ADDd) FON(FC); 
            else if(memory[RA]==255 && opcode==SUBd) FON(FN);
        }
        flg_zer();
    } else if(opcode>=ANDd && opcode<=XORd) {
        byte_t val=memory[TOD(b[0],b[1])];
        switch(opcode) {
            case ANDd:
                memory[RA]&=val;
                break;
            case ORd:
                memory[RA]|=val;
                break;
            case XORd:
                memory[RA]^=val;
                break;
        }
        flg_zer();
    } else if(opcode>=JMPd && opcode<=JNNd) {
        jmp_cas(opcode,b[0],b[1]);
    } else if(opcode==CLLd) {
        byte_t dd=memory[RPC];
        byte_t du=memory[RPC+1];
        dir_inc(&dd,&du);
        stk_psh(dd);
        stk_psh(du);
        memory[RPC]=b[0];
        memory[RPC+1]=b[1];
        FON(FJD);
    } return err_prt("Opcode not found",-2);
    return 0;
}

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
    memory[RPC]=IPR%256;
    memory[RPC+1]=IPR/256;
    memory[RHP]=IST%256;
    memory[RHP+1]=IST/256;
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

void mem_prt() {
    printf("SIZE=%i\n",DMEM);
    byte_t* p=memory;
    while(p!=memory+DMEM) printf("%03i ",*p++);
    printf("\n");
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
                if(!KION(KPA)) {
                    err=prg_exe();
                }
            }
        }
    }
    mem_prt();//dbg
    cmp_end();
    return err;
}
