#include "../inc/ppu.h"

#include "../inc/util.h"
#include "../inc/backend_sdl.h"

void ppu_init(Ppu* ppu, Bus* bus, uint32_t* buffer) {

	ppu->bus    = bus;
	ppu->pixels = buffer;
	ppu->dots   = 0;
}

#define LCDC_ENABLE 0x80
void ppu_step(Ppu* ppu, SDLContext* ctx, uint32_t cycles) {

	uint8_t lcdc = bus_read(ppu->bus, IO_LCDC);

	if (!(lcdc & LCDC_ENABLE)) {
		ppu->dots = 0;
		bus_write(ppu->bus, IO_LY, 0x00);
		return;
	}

	// handle OAM DMA transfers if ongoing

	/*
	uint8_t ly   = bus_read(ppu->bus, IO_LY);
	ppu->dots += cycles;

	while (ppu->dots >= 456) {
		ppu->dots -= 456;

		if (ly < 144) scanline_render(ppu);
		ly += 1;
		bus_write(ppu->bus, IO_LY, ly);

		// v_blank
		if (ly == 144) {
			bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | 0x01);
			SDL_UpdateWindowSurface(ctx->window);
		}

		if (ly > 153) {
			bus_write(ppu->bus, IO_LY, 0x00);
		}
	}
	*/
}

