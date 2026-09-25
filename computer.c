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
    c=c%4;
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
    const char* KEYS=KSU; //teclas utilizadas
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
    unsigned short dir=TOD(d,u);
    if(dir<DMEM) return memory+dir;
    return NULL;
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

static byte_t des_l(byte_t oc,byte_t ra) {
    if(ra & 128) FON(FC);
    ra=ra<<1;
    if(FION(FC) && oc==ROLA) ra|=1;
    return ra;
}

static byte_t des_r(byte_t oc,byte_t ra) {
    if(ra & 1) FON(FC);
    ra=ra>>1;
    if(FION(FC) && oc==RORA) ra|=128;
    return ra;
}

static int is_write(byte_t* d) {
    return (d && d>=memory+IRWM && d<memory+IRWM+DRWM+DORM);
}

static int is_read(byte_t* d) {
    return (d && d>=memory+IORM && d<memory+IORM+DORM);
}

static int ula(byte_t oc,byte_t bs,byte_t* b) {
    //calculos a partir de la entrada del opcode
    //todos los flags de calculo eliminados
    byte_t ra=memory[RA];
    byte_t rb=0;
    FOFF(FC|FZ|FN);
    int err=0;
    if(oc<20 && bs==0) {
        switch(oc) {
            case LDAX:
                ra=*(dir_get(memory[RX],memory[RX+1]));
                break;
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
            default:
                err=-2;
        }
    } else if(oc<30 && bs==1) {
        switch(oc) {
            case LDIA:
                ra=b[0];
                break;
            case CPIA:
                if(ra==b[0]) FON(FZ);
                else if(ra>b[0]) FON(FN);
                break;
            default:
                err=-2;
        }
    } else if(oc<50 && bs==2) {
        byte_t d[2];
        for(int k=0;k<1;k++) {
            dir_inc(memory+RPC,memory+RPC+1);
            d[k]=*(dir_get(memory[RPC],memory[RPC+1]));
        }
        byte_t* ptr=dir_get(d[0],d[1]);
        if(is_write(ptr)) {
            byte_t val=*ptr;
            switch(oc) {
                case LDAd:
                    ra=*ptr;
                    break;
                case ADDd:
                    unsigned short prea=val+ra;
                    if(prea<256) ra=prea;
                    else {
                        rb=prea/256;
                        ra=prea%256;
                        FON(FC);
                    }
                    break;
                case SUBd:
                    short pres=ra-val;
                    if(pres>=0) ra=pres;
                    else {
                        ra=pres;
                        FON(FN);
                    }
                    break;
                case ANDd:
                    ra=ra&val;
                    break;
                case ORd:
                    ra=ra|val;
                    break;
                case XORd:
                    ra=ra^val;
                    break;
                case POPA:
                    ra=stk_pop();
                    break;
                default:
                    err=-2;
            }
        }
    } else err=-2;
    if(err) err_prt("Opcode not found",err);
    else {
        if(ra==0) FON(FZ);
        memory[RA]=ra;
        memory[RB]=rb;
    }
    return err;
}

static int zero_byte(byte_t oc) {
    int err=0;
    switch(oc) {
        case HALT:
            KON(KQT);
            break;
        case WAIT:
            FON(FWAI);
            break;
        case STAX:
            *(dir_get(memory[RX],memory[RX+1]))=memory[RA];
            break;
        case INCX:
            dir_inc(memory+RX,memory+RX+1);
            break;
        case DECX:
            dir_dec(memory+RX,memory+RX+1);
            break;
        case PSHA:
            stk_psh(memory[RA]);
            break;
        case SWAB:
            byte_t c=memory[RA];
            memory[RA]=memory[RB];
            memory[RB]=c;
            break;
        case RET:
            memory[RPC+1]=stk_pop();
            memory[RPC]=stk_pop();
            FON(FJD);
            break;
        default:
            err=ula(oc,0,NULL);
    }
    return err;
}

static int one_byte(byte_t oc) {
    dir_inc(memory+RPC,memory+RPC+1);
    byte_t nb=*(dir_get(memory[RPC],memory[RPC+1]));
    int err=0;
    switch(oc) {
        default:
            byte_t arr[]={nb};
            err=ula(oc,1,arr);
    }
    return err;
}

static int jmp_is(byte_t oc) {
    int ret=0;
    switch(oc) {
        case JMPd:
            ret=1;
            break;
        case JFCd:
            ret=FION(FC);
            break;
        case JNCd:
            ret=!FION(FC);
            break;
        case JFZd:
            ret=FION(FZ);
            break;
        case JNZd:
            ret=!FION(FZ);
            break;
        case JFNd:
            ret=FION(FN);
            break;
        case JNNd:
            ret=!FION(FN);
            break;
    }
    return ret;
}

static int jmp(byte_t oc,byte_t* d) {
    byte_t* dir=dir_get(*d,*(d+1));
    if(dir>=memory+IPR && dir<memory+IPR+DPR) {
        if(jmp_is(oc)) {
            memory[RPC]=d[0];
            memory[RPC+1]=d[1];
            FON(FJD);
        }
    } else {
        return err_prt("JUMP out of memory",-5);
    }
    return 0;
}

static int two_byte(byte_t oc) {
    int err=0;
    byte_t ab[2];
    for(byte_t k=0;k<2;k++) {
        dir_inc(memory+RPC,memory+RPC+1);
        ab[k]=*(dir_get(memory[RPC],memory[RPC+1]));
    }
    byte_t* dir=dir_get(ab[0],ab[1]);
    switch(oc) {
        case STAd:
            {
                if(is_write(dir)) *dir=memory[RA];
                else err=-4;
                break;
            }
        case LDXd:
            {
                if(is_write(dir)) {
                    memory[RX]=ab[0];
                    memory[RX+1]=ab[1];
                } else err=-4;
                break;
            }
        case JMPd:
        case JFCd:
        case JNCd:
        case JFZd:
        case JNZd:
        case JFNd:
        case JNNd:
            err=jmp(oc,ab);
            break;
        case CLLd:
            dir_inc(memory+RPC,memory+RPC+1);
            stk_psh(memory[RPC]);
            stk_psh(memory[RPC+1]);
            err=jmp(JMPd,ab);
            break;
        default:
            err=ula(oc,2,ab);
    }
    if(err==-4) return err_prt("Writing out of memory",-4);
    return err;
}

static int prg_exe() {
    //ejecucion del programa
    int err=0;
    byte_t* doc=dir_get(memory[RPC],memory[RPC+1]);
    byte_t oc=*doc;
    printf("opcode=%i en %li\n",oc,doc-memory);//dbg
    if(oc<20) err=zero_byte(oc);
    else if(oc<30) err=one_byte(oc);
    else err=two_byte(oc);
    if(!err) { 
        if(FION(FJD)) FOFF(FJD);
        else {
            dir_inc(memory+RPC,memory+RPC+1);
            byte_t* ptr=dir_get(memory[RPC],memory[RPC+1]);
            if(ptr>=memory+IPR+DPR) {
                err=err_prt("Out of program bounds",-3);
            }
        }
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
    FON(FZ);//se conecta el flag cero nada mas empezar
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
        if(memory[p]==0) printf("--- ");
        else printf("%03i ",memory[p]);
    }
    puts("");
}

void mem_prt() {
    puts("REGISTROS");
    sec_prt(IRG,DRG);
    puts("PILA");
    sec_prt(IST,DST);
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
    cmp_end();
    return err;
}
