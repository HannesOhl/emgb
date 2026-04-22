#include "../inc/ppu.h"

#include "../inc/util.h"
#include "../inc/backend_sdl.h"

void ppu_init(Ppu* ppu, Bus* bus, uint32_t* buffer) {

	ppu->bus = bus;
	ppu->pixels = buffer;
	ppu->dots = 0;
	ppu->ly = 0;
}

static uint32_t palette_color(uint8_t shade) {

	switch (shade) {
		case 0: return 0xFFFFFFFF;
		case 1: return 0xFFAAAAAA;
		case 2: return 0xFF555555;
		case 3: return 0xFF000000;
		default: die("invalid case in palette_color!\n");
	}
	return 0xFFFF00FF;
}

static uint8_t bg_pixel(Ppu* ppu, uint32_t x, uint32_t y) {

	uint8_t scx  = bus_read(ppu->bus, 0xFF43);
	uint8_t scy  = bus_read(ppu->bus, 0xFF42);
	uint8_t lcdc = bus_read(ppu->bus, 0xFF40);

	uint16_t gx = (uint8_t) (x + scx);
	uint16_t gy = (uint8_t) (y + scy);

	uint16_t tile_x = gx / 8;
	uint16_t tile_y = gy / 8;

	uint16_t map_base = (lcdc & (1 << 3)) ? 0x9C00 : 0x9800;
	uint16_t map_offset = (uint16_t) (tile_y * 32 + tile_x);

	uint8_t tile_index = bus_read(ppu->bus, (uint16_t) (map_base) + map_offset);

	uint16_t tile_addr;

	if (lcdc & (1 << 4)) {
		tile_addr = (uint16_t) (0x8000 + (uint16_t) tile_index * 16);
	} else {
		int8_t signed_index = (int8_t) tile_index;
		tile_addr = (uint16_t) (0x9000 + signed_index * 16);
	}
	int32_t row = gy & 7;

	uint8_t low  = bus_read(ppu->bus, (uint16_t) (tile_addr + row * 2));
	uint8_t high = bus_read(ppu->bus, (uint16_t) (tile_addr + row * 2 + 1));

	int32_t bit = 7 - (gx & 7);

	return ((high >> bit) & 1) << 1 | ((low  >> bit) & 1);
}

static void scanline_render(Ppu* ppu) {
	uint8_t lcdc = bus_read(ppu->bus, 0xFF40);
	uint8_t bgp  = bus_read(ppu->bus, 0xFF47);

	// fill white
	if (!(lcdc & 0x01)) {
		for (size_t x = 0; x < (size_t) SCREEN_WIDTH; x++) {
		//for (size_t x = 0; x < (size_t) SCREEN_WIDTH*SCREEN_HEIGHT; x++) {
			ppu->pixels[x + ppu->ly* (size_t) SCREEN_WIDTH] = 0xFFFFFFFF;
			//ppu->pixels[x] = 0xFFFFFFFF;
		}
		return;
	}

	for (size_t x = 0; x < (size_t) SCREEN_WIDTH; x++) {
		uint8_t color_id = bg_pixel(ppu, (uint32_t) x, ppu->ly);
		uint8_t shade = (bgp >> color_id * 2) & 0x3;
		ppu->pixels[x + ppu->ly * (size_t) SCREEN_WIDTH] = palette_color(shade);
	}
}

void ppu_step(Ppu* ppu, SDLContext* ctx, uint32_t cycles) {

	uint8_t lcdc = bus_read(ppu->bus, 0xFF40);

	if (!(lcdc & 0x80)) {
		ppu->dots = 0;
		ppu->ly = 0;
		bus_write(ppu->bus, 0xFF44, 0);
		return;
	}

	ppu->dots += cycles;

	while (ppu->dots >= 456) {
		ppu->dots -= 456;

		if (ppu->ly < 144) scanline_render(ppu);
		ppu->ly += 1;
		bus_write(ppu->bus, 0xFF44, ppu->ly);

		// v_blank
		if (ppu->ly == 144) {
			bus_write(ppu->bus, 0xFF0F, bus_read(ppu->bus, 0xFF0F) | 0x01);
			SDL_UpdateWindowSurface(ctx->window);
		}

		if (ppu->ly > 153) {
			ppu->ly = 0;
			bus_write(ppu->bus, 0xFF44, 0);
		}
	}
}

