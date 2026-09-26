#include "ensamblador.h"

#define EOS '\0'

struct token_s {
    signed char opc;
    char cmp;
    unsigned char dir;
    unsigned short val;
    struct token_s* next;
};

static char* word_ini(char* str) {
    //da el inicio de una palabra
    char* ptr=str;
    while(*ptr==' ' && *ptr!=EOS) ptr++;
    if(*ptr==EOS) return NULL;
    else return ptr;
}

static char* word_set(char* str,char* word) {
    char* ptr=str;
    char* pw=word;
    while(*ptr!=EOS && *ptr!=' ') {
        *pw++=*ptr++;
    }
    *pw=EOS;
    return ptr;
}

static int word_equa(const char* a,char* b) {
    const char* pa=a;
    char* pb=b;
    while(*pa!=EOS) {
        if(*pa!=*pb) return 0;
        pa++;
        pb++;
    }
    return (*pb==EOS);
}

static int opc_set(char* word,token_t* token) {
    const char* OPC[]={LOADS,STATS,INCS,DECS,NOTS,SHLS,SHRS,ROLS,RORS,PSHS,POPS,RETS,SWAS,CMPS,ADDS,SUBS,ANDS,ORS,XORS,JMS,JMNS,CLLS};
    const unsigned char OPCS=22;
    token->opc=-1;
    for(unsigned char k=0;k<OPCS;k++) {
        if(word_equa(OPC[k],word)) {
            token->opc=k;
            break;
        }
    }
    if(token->opc==-1) {
        fprintf(stderr,"Opcode %s not recognised\n",word);
        return 1;
    }
    return 0;
}

static int cmp_set(char c,token_t* token) {
    const char* CMP="AXZCN";
    const char* p=CMP;
    token->cmp=0;
    while(*p!=EOS) {
        if(*p==c) {
            token->cmp=c;
            break;
        }
        p++;
    }
    if(token->cmp==0) {
        fprintf(stderr,"Complement %c not recognised\n",c);
        return 2;
    }
    return 0;
}

static int val_dir(char* word,unsigned short* val) {
    int dir=0;
    char nword[10];
    char* pn=nword;
    char* p=word;
    if(*p=='[') {
        dir=1;
        p++;
    }
    while(*p!=EOS && *p!=']') {
        *pn++=*p++;
    }
    *pn=EOS;
    sscanf(nword,"%hi",val);
    return dir;
}

static void val_set(char* word,token_t* token) {
    token->dir=val_dir(word,&token->val);
}

static int sentence_set(char* sentence,token_t* token) {
    char word[10];
    char* ps=sentence;
    int err=0;
    if((ps=word_ini(ps))) {
        ps=word_set(ps,word); //numero ignorado
        if((ps=word_ini(ps))) {
            ps=word_set(ps,word);
            err=opc_set(word,token);
            if(err!=0) return err;
            if((ps=word_ini(ps))) {
                ps=word_set(ps,word);
                err=cmp_set(*word,token);
                if(err!=0) return err;
                if((ps=word_ini(ps))) {
                    ps=word_set(ps,word);
                    val_set(word,token);
                }
            }
        }
    }
    return err;
}

static int 


    



     





