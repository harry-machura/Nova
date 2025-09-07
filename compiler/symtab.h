#ifndef NOVA_SYMTAB_H
#define NOVA_SYMTAB_H

// Simple symbol table API for future use.
// Current novac.c does not depend on this yet; it's provided for gradual refactor.

#ifdef __cplusplus
extern "C" {
#endif

void  sym_reset(void);
void  scope_push(void);
void  scope_pop(void);
int   sym_lookup_slot(const char* name); // -1 if not found
int   sym_declare(const char* name);     // returns slot index 0..255

#ifdef __cplusplus
}
#endif
#endif
