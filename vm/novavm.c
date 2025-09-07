
// nova - minimal VM with string pool and print/println
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum {
    OP_HALT=0, OP_PUSHI, OP_PUSHSTR,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE,
    OP_AND, OP_OR, OP_NOT,
    OP_JMP, OP_JZ,
    OP_LOAD, OP_STORE,
    OP_CALL, OP_RET, OP_ARG,
    OP_PRINT, OP_PRINTLN
};

/* Einheitliche Program-Struktur für die VM */
typedef struct Program {
    uint32_t nstrs;   /* Anzahl Strings im Konstantenpool */
    char   **strs;    /* String-Tabelle (Konstantenpool)   */
    uint8_t *code;    /* Bytecode                          */
    uint32_t code_len;/* Länge des Bytecodes               */
} Program;

static void free_program(Program* pr);

static int32_t read_i32(const uint8_t* p){ return (int32_t)( (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24) ); }

// ---- vm/novavm.c ----
// Ersetzt die defekte load_program-Funktion 1:1
static Program* load_program(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { perror("fopen"); return NULL; }

    Program* pr = (Program*)calloc(1, sizeof(Program));
    if (!pr) { fclose(f); return NULL; }

    /* Magic / Header (8 Bytes) einlesen */
    char magic[9] = {0};
    if (fread(magic, 1, 8, f) != 8) {
        fprintf(stderr, "read error (magic)\n");
        free(pr); fclose(f); return NULL;
    }

    /* sehr toleranter Check (passe an, falls dein Compiler ein fixes 8-Byte-Tag nutzt) */
    if (memcmp(magic, "NOVA", 4) != 0 && memcmp(magic, "NOVABC", 6) != 0) {
        fprintf(stderr, "bad magic: '%s'\n", magic);
        free(pr); fclose(f); return NULL;
    }

    /* String-Konstanten */
    uint32_t nstrs = 0;
    if (fread(&nstrs, 4, 1, f) != 1) {
        fprintf(stderr, "read error (nstrs)\n");
        free(pr); fclose(f); return NULL;
    }
    pr->nstrs = nstrs;

    if (nstrs > 0) {
        pr->strs = (char**)calloc(nstrs, sizeof(char*));
        if (!pr->strs) { free(pr); fclose(f); return NULL; }

        for (uint32_t i = 0; i < nstrs; ++i) {
            uint32_t len = 0;
            if (fread(&len, 4, 1, f) != 1) {
                fprintf(stderr, "read error (str len)\n");
                free_program(pr); fclose(f); return NULL;
            }

            char* s = (char*)malloc(len + 1);
            if (!s) { free_program(pr); fclose(f); return NULL; }

            if (len > 0 && fread(s, 1, len, f) != len) {
                fprintf(stderr, "read error (str data)\n");
                free(s); free_program(pr); fclose(f); return NULL;
            }
            s[len] = 0;
            pr->strs[i] = s;
        }
    }

    /* Bytecode */
    uint32_t code_len = 0;
    if (fread(&code_len, 4, 1, f) != 1) {
        fprintf(stderr, "read error (code_len)\n");
        free_program(pr); fclose(f); return NULL;
    }
    pr->code_len = code_len;

    if (code_len > 0) {
        pr->code = (uint8_t*)malloc(code_len);
        if (!pr->code) { free_program(pr); fclose(f); return NULL; }

        if (fread(pr->code, 1, code_len, f) != code_len) {
            fprintf(stderr, "read error (code data)\n");
            free_program(pr); fclose(f); return NULL;
        }
    }

    fclose(f);
    return pr;
}





static void free_program(Program* pr) {
    if (!pr) return;

    if (pr->strs) {
        for (uint32_t i = 0; i < pr->nstrs; ++i) {
            free(pr->strs[i]);
        }
        free(pr->strs);
    }

    free(pr->code);
    free(pr);
}

int main(int argc, char** argv){
    if(argc<2){ fprintf(stderr,"Usage: %s <program.nvc> [args]\n", argv[0]); return 2; }
    Program* pr = load_program(argv[1]);
    if(!pr) return 1;

    int32_t stack[2048]; int sp=0;
    int32_t vars[256]; memset(vars,0,sizeof(vars));

    uint8_t* code = pr->code; uint32_t pc=0, n=pr->code_len;
    #define POP()    (stack[--sp])
    #define PUSH(x)  (stack[sp++]=(x))
    #define FETCHI32() ({ int32_t _v = read_i32(&code[pc]); pc+=4; _v; })
    int32_t fp_stack[256];  int fsp = 0;
    uint32_t rp_stack[256]; int rsp = 0;
    int32_t fp = 0; 
    while(pc<n){
        uint8_t op = code[pc++];
        switch(op){
            case OP_HALT: free_program(pr); return 0;
            case OP_PUSHI: { int32_t v = FETCHI32(); PUSH(v); } break;
            case OP_PUSHSTR: {
                int32_t id = FETCHI32();
                // we push the id as int; printing will detect via separate opcode path.
                PUSH(0x40000000 | id); // tag top bit-range to mark string id (simple tagged int)
            } break;
            case OP_ADD: { int32_t b=POP(), a=POP(); PUSH(a+b); } break;
            case OP_SUB: { int32_t b=POP(), a=POP(); PUSH(a-b); } break;
            case OP_MUL: { int32_t b=POP(), a=POP(); PUSH(a*b); } break;
            case OP_DIV: { int32_t b=POP(), a=POP(); if(b==0){ fprintf(stderr,"division by zero\n"); free_program(pr); return 1;} PUSH(a/b); } break;
            case OP_MOD: { int32_t b=POP(), a=POP(); if(b==0){ fprintf(stderr,"mod by zero\n"); free_program(pr); return 1;} PUSH(a%b); } break;
            case OP_EQ:  { int32_t b=POP(), a=POP(); PUSH(a==b); } break;
            case OP_NE:  { int32_t b=POP(), a=POP(); PUSH(a!=b); } break;
            case OP_LT:  { int32_t b=POP(), a=POP(); PUSH(a<b); } break;
            case OP_LE:  { int32_t b=POP(), a=POP(); PUSH(a<=b); } break;
            case OP_GT:  { int32_t b=POP(), a=POP(); PUSH(a>b); } break;
            case OP_GE:  { int32_t b=POP(), a=POP(); PUSH(a>=b); } break;
            case OP_AND: { int32_t b=POP(), a=POP(); PUSH((a!=0)&&(b!=0)); } break;
            case OP_OR:  { int32_t b=POP(), a=POP(); PUSH((a!=0)||(b!=0)); } break;
            case OP_NOT: { int32_t a=POP(); PUSH(!a); } break;
            case OP_JMP: { int32_t off = FETCHI32(); pc = (uint32_t)((int32_t)pc + off); } break;
            case OP_JZ:  { int32_t off = FETCHI32(); int32_t v=POP(); if(v==0) pc = (uint32_t)((int32_t)pc + off); } break;
            case OP_LOAD:{ int32_t slot=FETCHI32(); PUSH(vars[slot]); } break;
            case OP_STORE:{ int32_t slot=FETCHI32(); vars[slot]=POP(); } break;
            case OP_PRINT:
            case OP_PRINTLN:{
                int32_t v = POP();
                if((v & 0x40000000) && !(v & 0x80000000)){ // tagged string id (simple check)
                    int id = v & 0x3FFFFFFF;
                    if(id<0 || (uint32_t)id>=pr->nstrs){ fprintf(stderr,"bad string id\n"); free_program(pr); return 1; }
                    fputs(pr->strs[id], stdout);
                } else {
                    printf("%d", v);
                }
                if(op==OP_PRINTLN) fputc('\n', stdout);
            } break;
            case OP_CALL: {
    uint32_t tgt = (uint32_t)FETCHI32();   // absolute Code-Adresse (Offset im Bytecode)
    int32_t argc = FETCHI32();
    // push aktuelle Frame-/Return-Infos
    fp_stack[fsp++] = fp;
    rp_stack[rsp++] = pc;
    // Neues Frame beginnt bei (sp - argc)
    fp = sp - argc;
    // Sprung in Funktion
    pc = tgt;
} break;

case OP_RET: {
    int32_t has_val = FETCHI32();  // 0 oder 1
    int32_t retv = 0;
    if (has_val) retv = POP();
    // Stack zurückrollen: Argumente entfernen
    sp = fp;
    // Frame/Return wiederherstellen
    fp = fp_stack[--fsp];
    pc = rp_stack[--rsp];
    if (has_val) PUSH(retv);
} break;

case OP_ARG: {
    int32_t idx = FETCHI32();    // 0..argc-1
    PUSH(stack[fp + idx]);
} break;

            default:
                fprintf(stderr,"unknown opcode %u at pc=%u\n", op, pc-1);
                free_program(pr); return 1;
        }
    }
    free_program(pr);
    return 0;
}
