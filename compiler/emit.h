#ifndef NOVA_EMIT_H
#define NOVA_EMIT_H
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* data;
    size_t   len;
    size_t   cap;
} CodeBuf;

void cb_init(CodeBuf* b);
void cb_free(CodeBuf* b);
void cb_w8(CodeBuf* b, uint8_t v);
void cb_w32(CodeBuf* b, int32_t v);

#endif
