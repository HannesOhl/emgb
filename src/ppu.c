#include "../inc/ppu.h"

#include "../inc/util.h"
#include "../inc/backend_sdl.h"

#define LCDC_ENABLE 0x80
#define OAM_SIZE 160
static bool line_rendered = false;

static uint32_t bw_palette[4] = {
    0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000
};

static inline uint32_t palette_color(uint8_t shade) {
    return bw_palette[shade & 3];
}

void ppu_init(Ppu* ppu, Bus* bus, uint32_t* buffer) {

	ppu->bus    = bus;
	ppu->pixels = buffer;
	ppu->dots   = 0;
}

void interrupt_send(Bus* bus, uint8_t n) {
	uint8_t flag = bus_read(bus, IO_IF);
	flag |= (1 << n);
	bus_write(bus, IO_IF, flag);
}

static inline uint8_t shade_from_palette(uint8_t palette, uint8_t color_index) {
	return (uint8_t) ((palette >> (color_index * 2u)) & 0x03);
}

static inline uint16_t tile_data_addr(uint8_t lcdc, uint8_t tile_index) {
	if (lcdc & (1 << 4)) return (uint16_t) (0x8000 + (uint16_t) tile_index * 16);
    	return (uint16_t) (0x9000 + (int16_t) ((int8_t) tile_index * 16));
}

static uint8_t pixel_index(Ppu *ppu, uint16_t base, uint16_t sx, uint16_t sy) {

	Bus* bus = ppu->bus;

	uint8_t lcdc = bus_read(bus, IO_LCDC);

	// wrap to 8-bit BG/window space
	uint8_t x = (uint8_t) sx;
	uint8_t y = (uint8_t) sy;

	uint16_t tile_x   = (uint16_t) (x >> 3);
	uint16_t tile_y   = (uint16_t) (y >> 3);
	uint16_t map_addr = (uint16_t) (base + tile_y * 32 + tile_x);

	uint8_t tile_index = bus_read(bus, map_addr);
	uint16_t tile_addr = tile_data_addr(lcdc, tile_index);

	uint8_t row = (uint8_t) (y & 7);
	uint8_t col = (uint8_t) (x & 7);

	uint16_t row_addr = (uint16_t) (tile_addr + (uint16_t) row * 2);

	uint8_t low  = bus_read(bus, row_addr);
	uint8_t high = bus_read(bus, (uint16_t) (row_addr + 1));

	uint8_t bit = (uint8_t) (7 - col);

	uint8_t pixel_index = ( ((high >> bit) & 1) << 1 ) | ( (low  >> bit) & 1 );

	return pixel_index;
}

void line_render(Ppu* ppu, uint8_t ly) {

	Bus* bus = ppu->bus;

	uint8_t lcdc = bus_read(bus, IO_LCDC);
	uint8_t bgp  = bus_read(bus, IO_BGP);

	// for background
	uint8_t scx = bus_read(bus, IO_SCX);
	uint8_t scy = bus_read(bus, IO_SCY);
	uint16_t base_b = (lcdc & (1u << 3)) ? 0x9C00 : 0x9800;

	// for window
	uint8_t wy = bus_read(bus, IO_WY);
	uint8_t wx = bus_read(bus, IO_WX);
	int16_t wx_origin = (int16_t) wx - 7;
	uint16_t base_w = (lcdc & (1 << 6)) ? 0x9C00 : 0x9800;

	bool w = (lcdc & 0x20) && (ly >= wy) && (wx_origin < 160);
	for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {

		uint8_t color_index = 0;

		if (lcdc & 0x01) {
			color_index = pixel_index(ppu, base_b, (uint16_t) (x + scx), (uint16_t) (ly + scy));
		}

		// override bg
        	if (w && (int16_t) x >= wx_origin) {
			color_index = pixel_index(ppu, base_w, (uint16_t) ((int16_t) x - wx_origin), ppu->window_line);
        	}

        	ppu->pixels[x + ly * SCREEN_WIDTH] = palette_color(shade_from_palette(bgp, color_index));
    	}

	if (w) ppu->window_line++;
	line_rendered = true;
}

void ppu_step(Ppu* ppu, SDLContext* ctx, uint32_t cycles) {

	Bus* bus = ppu->bus;
	uint8_t lcdc = bus_read(bus, IO_LCDC);

	if (!(lcdc & LCDC_ENABLE)) {
		ppu->dots = 0;
		ppu->window_line = 0;
		bus_write(bus, IO_LY, 0x00);
		return;
	}

	ppu->dots += cycles;

	// handle OAM DMA transfers if ongoing
	uint8_t dma = bus_read(bus, IO_DMA);
	if (dma != 0) {
		uint16_t dma_src_addr = dma << 8;
		for (size_t i = 0; i < OAM_SIZE; i++) {
			uint8_t dma_src = bus_read(bus, dma_src_addr + (uint16_t) i);
			bus_write(bus, 0xFE00 + (uint16_t) i, dma_src);
		}
		bus_write(bus, IO_DMA, 0x00);
	}

	uint8_t mode = bus_read(bus, IO_STAT) & 0x03;

	// local hw regs, write back later
	uint8_t ly = bus_read(bus, IO_LY);
	uint8_t lyc = bus_read(bus, IO_LYC);
	uint8_t stat = bus_read(bus, IO_STAT);

	if (mode == 1) {
		if (ppu->dots >= 456) {
			ppu->dots -= 456;
			ly += 1;
			line_rendered = false;
			if (ly >= 154) {
				stat = (stat & (uint8_t) ~0x03) | 0x02;
				ly = 0;
				ppu->window_line = 0;
				if (stat & 0x20) interrupt_send(bus, 1);
			}

			if (ly == lyc) {
				stat |= 0x04;
				if (stat & 0x40) interrupt_send(bus, 1);
			} else {
				stat &= (uint8_t) ~0x04;
			}
		}
		bus_write(bus, IO_LY  , ly);
		bus_write(bus, IO_STAT, stat);
		return;
	}

	if (ppu->dots <= 79) {
		// mode 2, read OAM
		stat = (stat & (uint8_t) ~0x03) | 0x02;

		if (mode != 2 && (stat & 0x20)) interrupt_send(bus, 1);

	} else if (ppu->dots <= 251) {
		// mode 3, read OAM and VRAM
		stat = (stat & (uint8_t) ~0x03) | 0x03;

		if (!line_rendered) line_render(ppu, ly);

	} else if (ppu->dots <= 455) {
		// mode 0, HBlank
		stat &= (uint8_t) ~0x03;

		if (mode != 0 && (stat & 0x08)) interrupt_send(bus, 1);
	} else {
		// horizontal line complete
		ppu->dots -= 456;
		ly += 1;
		line_rendered = false;

		if (ly >= 144) {
			stat = (stat & (uint8_t) ~0x03) | 0x01;
			interrupt_send(bus, 0);
			SDL_UpdateWindowSurface(ctx->window);
		} else {
			stat = (stat & (uint8_t) ~0x03) | 0x02;
			if (mode != 2 && (stat & 0x20)) interrupt_send(bus, 1);
		}

		if (ly == lyc) {
			stat |= 0x04;
			if (stat & 0x40) interrupt_send(bus, 1);
		} else {
			stat &= (uint8_t) ~0x04;
		}
	}
	bus_write(bus, IO_LY  , ly);
	bus_write(bus, IO_STAT, stat);
}

