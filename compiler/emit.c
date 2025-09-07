#include "emit.h"
#include <stdlib.h>
#include <string.h>

static void ensure(CodeBuf* b, size_t need){
    size_t want = b->len + need;
    if (want <= b->cap) return;
    size_t ncap = b->cap ? b->cap * 2 : 1024;
    while (ncap < want) ncap *= 2;
    uint8_t* nbuf = (uint8_t*)realloc(b->data, ncap);
    if (!nbuf) { abort(); }
    b->data = nbuf;
    b->cap  = ncap;
}

void cb_init(CodeBuf* b){ b->data=NULL; b->len=0; b->cap=0; }
void cb_free(CodeBuf* b){
    if (!b) return;
    if (b->data) { free(b->data); b->data=NULL; }
    b->len=0; b->cap=0;
}

void cb_w8(CodeBuf* b, uint8_t v){ ensure(b,1); b->data[b->len++] = v; }

void cb_w32(CodeBuf* b, int32_t v){
    ensure(b,4);
    b->data[b->len+0] = (uint8_t)( v        & 0xFF);
    b->data[b->len+1] = (uint8_t)((v >> 8 ) & 0xFF);
    b->data[b->len+2] = (uint8_t)((v >> 16) & 0xFF);
    b->data[b->len+3] = (uint8_t)((v >> 24) & 0xFF);
    b->len += 4;
}
