#ifndef NOVA_DIAG_H
#define NOVA_DIAG_H
#include <stdio.h>
#include <stdlib.h>

struct Lexer; // forward-declare to match novac.c

static inline void die(const char* msg){
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

static inline void die_at(struct Lexer* L, const char* msg){
    // We can't access fields of Lexer here (opaque), so print generic form.
    // novac.c already passes useful messages.
    (void)L;
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

#endif
