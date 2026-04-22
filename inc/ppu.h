#ifndef PPU_H
#define PPU_H

#include "./bus.h"

#include <stdint.h>

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

typedef struct {
	Bus* bus;

	uint32_t dots;
	uint8_t ly;

	uint32_t* pixels;
} Ppu;

void ppu_init(Ppu* ppu, Bus* bus, uint32_t* pixels);
void ppu_step(Ppu* ppu, uint32_t cycels);

#endif

