#include "../inc/ppu.h"

#include "../inc/util.h"
#include "../inc/backend_sdl.h"

void ppu_init(Ppu* ppu, Bus* bus, uint32_t* buffer) {

	ppu->bus    = bus;
	ppu->pixels = buffer;
	ppu->dots   = 0;
}

#define LCDC_ENABLE 0x80
#define OAM_SIZE 160
void ppu_step(Ppu* ppu, SDLContext* ctx, uint32_t cycles) {

	Bus* bus = ppu->bus;
	uint8_t lcdc = bus_read(bus, IO_LCDC);

	if (!(lcdc & LCDC_ENABLE)) {
		ppu->dots = 0;
		bus_write(bus, IO_LY, 0x00);
		return;
	}

	ppu->dots += cycles;

	// handle OAM DMA transfers if ongoing
	uint8_t dma = bus_read(bus, IO_DMA);
	if (dma != 0) {
		uint16_t dma_src_addr = dma << 8;
		for (size_t i = 0; i < OAM_SIZE; i++) {
			uint8_t dma_src = bus_read(bus, dma_src_addr + i);
			bus_write(bus, 0xFE00 + i, dma_src);
		}
		bus_write(bus, IO_DMA, 0x00);
	}

	uint8_t mode = bus_read(bus, IO_STAT) & 0x03;


	if (mode == 1) {

		return;
	}

	if (ppu->dots <= 79) {
		// mode 2
		// mode 3
	} else if (ppu->dots <= 251) {
		// mode 3
	} else if (ppu->dots <= 455) {
		// mode 0
		// mode 2
		// mode 3
	} else {
		// horizontal line complete
	}
}

	/*
	uint8_t ly   = bus_read(ppu->bus, IO_LY);

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

