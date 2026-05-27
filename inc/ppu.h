#ifndef PPU_H
#define PPU_H

#include "./bus.h"
#include "./backend.h"

#include <stdint.h>

typedef struct {
	uint32_t dots;
	uint8_t window_line;
	uint32_t* pixels;
} Ppu;

bool ppu_init(Ppu* ppu);
void ppu_step(Ppu* ppu, Bus* bus, EXTBackendContext* ctx, uint32_t cycles);

#endif

