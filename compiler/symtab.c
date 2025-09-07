#include "symtab.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NOVA_MAX_SLOTS   256
#define NOVA_MAX_SYMBOLS 1024

typedef struct {
    char     name[64];
    uint8_t  slot;
    int      depth;
    int      in_use;
} SymEnt;

static SymEnt g_symbols[NOVA_MAX_SYMBOLS];
static int    g_sym_count = 0;
static int    g_scope_depth = 0;
static int    g_next_slot   = 0;

void sym_reset(void){
    memset(g_symbols, 0, sizeof(g_symbols));
    g_sym_count=0; g_scope_depth=0; g_next_slot=0;
}

void scope_push(void){ g_scope_depth++; }

void scope_pop(void){
    for (int i=g_sym_count-1; i>=0; --i){
        if (g_symbols[i].in_use && g_symbols[i].depth == g_scope_depth){
            g_symbols[i].in_use = 0;
        }
    }
    g_scope_depth--;
    if (g_scope_depth < 0) g_scope_depth = 0;
}

int sym_lookup_slot(const char* name){
    for (int i=g_sym_count-1; i>=0; --i){
        if (g_symbols[i].in_use && strcmp(g_symbols[i].name, name) == 0){
            return g_symbols[i].slot;
        }
    }
    return -1;
}

int sym_declare(const char* name){
    if (g_next_slot >= NOVA_MAX_SLOTS){
        fprintf(stderr, "out of variable slots (max %d)\n", NOVA_MAX_SLOTS);
        exit(1);
    }
    if (g_sym_count >= NOVA_MAX_SYMBOLS){
        fprintf(stderr, "symbol table full (max %d)\n", NOVA_MAX_SYMBOLS);
        exit(1);
    }
    SymEnt* e = &g_symbols[g_sym_count++];
    memset(e, 0, sizeof(*e));
    strncpy(e->name, name, sizeof(e->name)-1);
    e->slot  = (uint8_t)g_next_slot++;
    e->depth = g_scope_depth;
    e->in_use= 1;
    return e->slot;
}
