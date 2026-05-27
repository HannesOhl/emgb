#ifndef BACKEND_H
#define BACKEND_H

#include "./input.h"

#include <stdint.h>

typedef struct EXTBackendContext EXTBackendContext;

EXTBackendContext* EXT_backend_init(uint32_t* buf);
void EXT_backend_present(EXTBackendContext* ctx);
void EXT_backend_destroy(EXTBackendContext* ctx);
bool EXT_backend_input(EXTBackendContext* ctx, InputEvent* e);

#endif

