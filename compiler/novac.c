
// nova - minimal compiler with string support
// Bytecode format:
// [magic "NOVABC01"][u32 str_count][each: u32 len + bytes][u32 code_size][code bytes]
// Variables: up to 256 slots (i32 values). Strings live in constant pool; VM prints strings/ints.
//
// Language subset:
//  program := { stmt }
//  stmt    := "let" ident "=" expr | ident "=" expr | "print" "(" expr ")" | "println" "(" expr ")" | if | while | "{" { stmt } "}"
//  if      := "if" "(" expr ")" block [ "else" block ]
//  while   := "while" "(" expr ")" block
//  expr    := precedence climbing over ||, &&, comparisons, + - * / %, unary - !
//  primary := number | string | ident | "(" expr ")"
//
// No semicolons; newlines and braces separate statements.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_CODE  (1<<20)
#define MAX_VARS  256
#define MAX_STRS  4096

typedef struct { const char* src; size_t len; size_t pos; int line; } Lexer;

typedef enum {
    T_EOF=0, T_IDENT, T_INT, T_STRING,
    T_LP='(', T_RP=')', T_LB='{', T_RB='}',
    T_EQ='=', T_PLUS='+', T_MINUS='-', T_STAR='*', T_SLASH='/', T_PCT='%',
    T_LT='<', T_GT='>', T_BANG='!',
    T_AMP='&', T_BAR='|',
    T_COMMA=',',
    // multi-char
    T_EQEQ=256, T_NEQ, T_LE, T_GE, T_ANDAND, T_OROR,
    // keywords
    K_LET, K_IF, K_ELSE, K_WHILE, K_PRINT, K_PRINTLN
} TokKind;

typedef struct { TokKind kind; char text[256]; int64_t ival; } Token;

typedef struct {
    uint8_t* data; size_t len;
} CodeBuf;

static void cb_init(CodeBuf* b){ b->data=(uint8_t*)malloc(MAX_CODE); b->len=0; }
static void cb_w8(CodeBuf* b, uint8_t v){ b->data[b->len++]=v; }
static void cb_w32(CodeBuf* b, int32_t v){
    for(int i=0;i<4;i++) cb_w8(b, (uint8_t)((v>> (8*i)) & 0xFF));
}

static void die(const char* msg){ fprintf(stderr,"error: %s\n", msg); exit(1); }
static void die_at(Lexer* L, const char* msg){ fprintf(stderr,"error(line %d): %s\n", L->line, msg); exit(1); }

static void lx_init(Lexer* L, const char* src){
    L->src=src; L->len=strlen(src); L->pos=0; L->line=1;
}
static int lx_peek(Lexer* L){ return (L->pos<L->len)? (unsigned char)L->src[L->pos]: -1; }
static int lx_get(Lexer* L){ int c=lx_peek(L); if(c==-1) return -1; L->pos++; if(c=='\n') L->line++; return c; }
static void lx_skip_ws(Lexer* L){
    for(;;){
        // --- NEW: UTF-8 BOM am Dateianfang überspringen ---
        if (L->pos == 0 && L->len >= 3 &&
            (unsigned char)L->src[0] == 0xEF &&
            (unsigned char)L->src[1] == 0xBB &&
            (unsigned char)L->src[2] == 0xBF) {
            L->pos = 3; // BOM überspringen
        }

        int c = lx_peek(L);

        // --- NEW: NBSP (0xA0) als Whitespace behandeln ---
        if (c == 0xA0) { lx_get(L); continue; }

        // Line-Kommentare //... bis zum Zeilenende
        if (c=='/' && L->pos+1<L->len && L->src[L->pos+1]=='/'){
            while((c=lx_get(L))!=-1 && c!='\n');
            continue;
        }
        if (isspace(c)) { lx_get(L); continue; }
        break;
    }
}


static int is_ident_start(int c){ return isalpha(c)||c=='_'; }
static int is_ident_cont(int c){ return isalnum(c)||c=='_'; }

static Token lx_next(Lexer* L){
    lx_skip_ws(L);
    Token t; t.kind=T_EOF; t.text[0]=0; t.ival=0;
    int c=lx_peek(L);
    if(c==-1){ t.kind=T_EOF; return t; }

    // strings
    if(c=='"'){
        lx_get(L);
        char buf[256]; int i=0;
        while((c=lx_get(L))!=-1 && c!='"'){
            if(c=='\\'){
                int n=lx_get(L);
                if(n=='n') c='\n';
                else if(n=='t') c='\t';
                else if(n=='"') c='"';
                else if(n=='\\') c='\\';
                else { die_at(L, "unknown escape"); }
            }
            if(i<255) buf[i++]= (char)c;
        }
        if(c!='"') die_at(L,"unterminated string");
        buf[i]=0;
        t.kind=T_STRING; strncpy(t.text, buf, sizeof(t.text));
        return t;
    }

    // numbers
if (isdigit(c)) {
    int64_t v = lx_get(L) - '0';            // <-- erstes Zeichen konsumieren
    while (isdigit(lx_peek(L))) { v = v*10 + (lx_get(L)-'0'); }
    t.kind = T_INT; t.ival = v; return t;
}

    // identifiers / keywords
    // identifiers / keywords
if (is_ident_start(c)) {
    int i = 0;
    t.text[i++] = (char)lx_get(L);          // <-- erstes Zeichen konsumieren
    while (is_ident_cont(lx_peek(L)) && i < 255) { t.text[i++] = (char)lx_get(L); }
    t.text[i] = 0;
    if (strcmp(t.text,"let")==0) t.kind=K_LET;
    else if (strcmp(t.text,"if")==0) t.kind=K_IF;
    else if (strcmp(t.text,"else")==0) t.kind=K_ELSE;
    else if (strcmp(t.text,"while")==0) t.kind=K_WHILE;
    else if (strcmp(t.text,"print")==0) t.kind=K_PRINT;
    else if (strcmp(t.text,"println")==0) t.kind=K_PRINTLN;
    else t.kind = T_IDENT;
    return t;
}


    // operators / punctuation
    c=lx_get(L);
    switch(c){
        case '(': t.kind=T_LP; break;
        case ')': t.kind=T_RP; break;
        case '{': t.kind=T_LB; break;
        case '}': t.kind=T_RB; break;
        case '+': t.kind=T_PLUS; break;
        case '-': t.kind=T_MINUS; break;
        case '*': t.kind=T_STAR; break;
        case '/': t.kind=T_SLASH; break;
        case '%': t.kind=T_PCT; break;
        case ',': t.kind=T_COMMA; break;
        case '!':
            if(lx_peek(L)=='='){ lx_get(L); t.kind=T_NEQ; }
            else t.kind=T_BANG;
            break;
        case '=':
            if(lx_peek(L)=='='){ lx_get(L); t.kind=T_EQEQ; }
            else t.kind=T_EQ;
            break;
        case '<':
            if(lx_peek(L)=='='){ lx_get(L); t.kind=T_LE; }
            else t.kind=T_LT;
            break;
        case '>':
            if(lx_peek(L)=='='){ lx_get(L); t.kind=T_GE; }
            else t.kind=T_GT;
            break;
        case '&':
            if(lx_peek(L)=='&'){ lx_get(L); t.kind=T_ANDAND; }
            else die_at(L,"single '&' not supported");
            break;
        case '|':
            if(lx_peek(L)=='|'){ lx_get(L); t.kind=T_OROR; }
            else die_at(L,"single '|' not supported");
            break;
        default: {
    char m[64];
    unsigned uc = (unsigned)c & 0xFF;
    snprintf(m, sizeof(m), "unexpected character '%c' (0x%02X)",
             (uc>=32&&uc<127)?uc:'?', uc);
    die_at(L, m);
}
    }
    return t;
}

// --------- Parser / Emitter ---------

enum {
    OP_HALT=0, OP_PUSHI, OP_PUSHSTR,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE,
    OP_AND, OP_OR, OP_NOT,
    OP_JMP, OP_JZ,
    OP_LOAD, OP_STORE,
    OP_PRINT, OP_PRINTLN
};

typedef struct {
    char name[64];
    int slot; // 0..255
} Var;

typedef struct {
    Var vars[MAX_VARS]; int nvars;
    char* strpool[MAX_STRS]; int nstrs;
} Env;

static int env_find_var(Env* E, const char* name){
    for(int i=0;i<E->nvars;i++) if(strcmp(E->vars[i].name,name)==0) return E->vars[i].slot;
    return -1;
}
static int env_add_var(Env* E, const char* name){
    if(E->nvars>=MAX_VARS) die("too many variables");
    int slot=E->nvars;
    snprintf(E->vars[E->nvars].name, sizeof(E->vars[0].name), "%s", name);
    E->vars[E->nvars].slot = slot;
    E->nvars++;
    return slot;
}
static int env_add_string(Env* E, const char* s){
    if(E->nstrs>=MAX_STRS) die("too many strings");
    E->strpool[E->nstrs] = strdup(s);
    return E->nstrs++;
}

// forward decls
typedef struct { Lexer* L; Token t; CodeBuf* out; Env* env; } P;
static void parse_stmt(P* p);
static void parse_block(P* p);
static void parse_expr(P* p);

static void next(P* p){ p->t = lx_next(p->L); }
static int accept(P* p, TokKind k){ if(p->t.kind==k){ next(p); return 1; } return 0; }
static void expect(P* p, TokKind k, const char* msg){ if(!accept(p,k)) die_at(p->L, msg); }

// Emitter helpers
static void emit(P* p, uint8_t op){ cb_w8(p->out, op); }
static void emit32(P* p, int32_t v){ cb_w32(p->out, v); }

// ---- Expressions ----
static void parse_primary(P* p){
    if(p->t.kind==T_INT){
        emit(p, OP_PUSHI); emit32(p, (int32_t)p->t.ival);
        next(p); return;
    }
    if(p->t.kind==T_STRING){
        int id = env_add_string(p->env, p->t.text);
        emit(p, OP_PUSHSTR); emit32(p, id);
        next(p); return;
    }
    if(p->t.kind==T_IDENT){
        char name[256]; strncpy(name, p->t.text, sizeof(name));
        next(p);
        int slot = env_find_var(p->env, name);
        if(slot<0){ char m[256]; snprintf(m,sizeof(m),"undefined variable '%.200s'", name); die_at(p->L, m); }
        emit(p, OP_LOAD); emit32(p, slot);
        return;
    }
    if(accept(p, T_LP)){
        parse_expr(p);
        expect(p, T_RP, "expected ')'");
        return;
    }
    die_at(p->L, "expected primary expression");
}

static void parse_unary_fixed(P* p){
    if(accept(p, T_MINUS)){
        // -(expr)  => push 0; expr; SUB
        emit(p, OP_PUSHI); emit32(p, 0);
        parse_unary_fixed(p);
        emit(p, OP_SUB);
        return;
    }
    if(accept(p, T_BANG)){
        parse_unary_fixed(p);
        emit(p, OP_NOT);
        return;
    }
    parse_primary(p);
}

static void parse_mul(P* p){
    parse_unary_fixed(p);
    for(;;){
        if(accept(p, T_STAR)){ parse_unary_fixed(p); emit(p, OP_MUL); }
        else if(accept(p, T_SLASH)){ parse_unary_fixed(p); emit(p, OP_DIV); }
        else if(accept(p, T_PCT)){ parse_unary_fixed(p); emit(p, OP_MOD); }
        else break;
    }
}

static void parse_add(P* p){
    parse_mul(p);
    for(;;){
        if(accept(p, T_PLUS)){ parse_mul(p); emit(p, OP_ADD); }
        else if(accept(p, T_MINUS)){ parse_mul(p); emit(p, OP_SUB); }
        else break;
    }
}

static void parse_cmp(P* p){
    parse_add(p);
    for(;;){
        if(accept(p, T_EQEQ)){ parse_add(p); emit(p, OP_EQ); }
        else if(accept(p, T_NEQ)){ parse_add(p); emit(p, OP_NE); }
        else if(accept(p, T_LT)){ parse_add(p); emit(p, OP_LT); }
        else if(accept(p, T_LE)){ parse_add(p); emit(p, OP_LE); }
        else if(accept(p, T_GT)){ parse_add(p); emit(p, OP_GT); }
        else if(accept(p, T_GE)){ parse_add(p); emit(p, OP_GE); }
        else break;
    }
}

static void parse_and(P* p){
    parse_cmp(p);
    while(accept(p, T_ANDAND)){
        // no short-circuit in MVP
        parse_cmp(p);
        emit(p, OP_AND);
    }
}

static void parse_or(P* p){
    parse_and(p);
    while(accept(p, T_OROR)){
        parse_and(p);
        emit(p, OP_OR);
    }
}

static void parse_expr(P* p){
    parse_or(p);
}

// ---- Statements ----
static void parse_stmt(P* p){
    if(accept(p, K_LET)){
        if(p->t.kind!=T_IDENT) die_at(p->L,"expected identifier after 'let'");
        char name[256]; strncpy(name, p->t.text, sizeof(name)); next(p);
        expect(p, T_EQ, "expected '=' after variable name");
        parse_expr(p);
        int slot = env_add_var(p->env, name);
        emit(p, OP_STORE); emit32(p, slot);
        return;
    }
    if(p->t.kind==T_IDENT){
        char name[256]; strncpy(name, p->t.text, sizeof(name)); next(p);
        expect(p, T_EQ, "expected '=' in assignment");
        parse_expr(p);
        int slot = env_find_var(p->env, name);
        if(slot<0){ char m[256]; snprintf(m,sizeof(m),"undefined variable '%s'", name); die_at(p->L, m); }
        emit(p, OP_STORE); emit32(p, slot);
        return;
    }
    if(accept(p, K_PRINT)){
        expect(p, T_LP, "expected '(' after print");
        parse_expr(p);
        expect(p, T_RP, "expected ')'");
        emit(p, OP_PRINT);
        return;
    }
    if(accept(p, K_PRINTLN)){
        expect(p, T_LP, "expected '(' after println");
        parse_expr(p);
        expect(p, T_RP, "expected ')'");
        emit(p, OP_PRINTLN);
        return;
    }
    if(accept(p, K_IF)){
        expect(p, T_LP, "expected '(' after if");
        parse_expr(p);
        expect(p, T_RP, "expected ')'");
        // JZ else
        emit(p, OP_JZ); size_t jz_pos = p->out->len; emit32(p, 0);
        parse_block(p);
        // JMP end
        emit(p, OP_JMP); size_t jmp_pos = p->out->len; emit32(p, 0);
        // patch JZ to jump here
        int32_t off_else = (int32_t)(p->out->len - jz_pos - 4);
        memcpy(p->out->data + jz_pos, &off_else, 4);
        if(accept(p, K_ELSE)){
            parse_block(p);
        }
        // patch JMP to end
        int32_t off_end = (int32_t)(p->out->len - jmp_pos - 4);
        memcpy(p->out->data + jmp_pos, &off_end, 4);
        return;
    }
    if(accept(p, K_WHILE)){
        expect(p, T_LP, "expected '(' after while");
        size_t cond_pos = p->out->len;
        parse_expr(p);
        expect(p, T_RP, "expected ')'");
        emit(p, OP_JZ); size_t jz_pos = p->out->len; emit32(p, 0);
        parse_block(p);
        // jump back to condition
        // jump back to the start of the condition
	emit(p, OP_JMP);
	{
	    // pc_after_operand = current_len + 4
	    int32_t back = (int32_t)cond_pos - (int32_t)(p->out->len + 4);
	    emit32(p, back);
	}

        // patch JZ to after block
        int32_t off = (int32_t)(p->out->len - jz_pos - 4);
        memcpy(p->out->data + jz_pos, &off, 4);
        return;
    }
    if(accept(p, T_LB)){
        // block
        while(p->t.kind!=T_RB && p->t.kind!=T_EOF){
            parse_stmt(p);
        }
        expect(p, T_RB, "expected '}'");
        return;
    }
    die_at(p->L, "unknown statement");
}

static void parse_block(P* p){
    if(!accept(p, T_LB)) die_at(p->L, "expected '{' to start block");
    while(p->t.kind!=T_RB && p->t.kind!=T_EOF){
        parse_stmt(p);
    }
    expect(p, T_RB, "expected '}'");
}

// File emission
static void write_u32(FILE* f, uint32_t v){
    for(int i=0;i<4;i++) fputc((v >> (8*i)) & 0xFF, f);
}

int main(int argc, char** argv){
    if(argc<3){
        fprintf(stderr,"Usage: %s <input.nova> <output.nvc>\n", argv[0]);
        return 2;
    }
    const char* in = argv[1];
    const char* out = argv[2];

    // read file
    FILE* fi = fopen(in, "rb"); if(!fi){ perror("open input"); return 1; }
    fseek(fi, 0, SEEK_END); long n=ftell(fi); fseek(fi,0,SEEK_SET);
    char* src = (char*)malloc(n+1);
    if(!src){ fprintf(stderr,"oom\n"); fclose(fi); return 1; }
    size_t rr = fread(src,1,n,fi); (void)rr; src[n]=0; fclose(fi);

    Lexer L; lx_init(&L, src);
    CodeBuf cb; cb_init(&cb);
    Env env; memset(&env,0,sizeof(env));
    P p = { &L, {0}, &cb, &env };
    next(&p);

    while(p.t.kind!=T_EOF){
        parse_stmt(&p);
    }
    emit(&p, OP_HALT);

    FILE* fo = fopen(out, "wb"); if(!fo){ perror("open output"); return 1; }
    fwrite("NOVABC01",1,8,fo);
    // strings
    write_u32(fo, (uint32_t)env.nstrs);
    for(int i=0;i<env.nstrs;i++){
        uint32_t len = (uint32_t)strlen(env.strpool[i]);
        write_u32(fo, len);
        fwrite(env.strpool[i],1,len,fo);
    }
    // code
    write_u32(fo, (uint32_t)cb.len);
    fwrite(cb.data,1,cb.len,fo);
    fclose(fo);
    return 0;
}
