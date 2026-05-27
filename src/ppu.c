#include "../inc/ppu.h"

// for calloc
#include <stdlib.h>
// for size_t
#include <stddef.h>

#define LCDC_ENABLE 			0x80
#define LCDC_BACKGROUND_ENABLE 		0x01
#define LCDC_SPRITE_ENABLE 		0x02
#define LCDC_SPRITE_SIZE   		0x04
#define LCDC_BACKGROUND_MAP_SELECT 	0x08
#define LCDC_WINDOW_ENABLE 		0x20
#define LCDC_WINDOW_MAP 		0x40

#define MODE_HBLANK 0
#define MODE_VBLANK 1
#define MODE_OAM    2
#define MODE_VRAM   3

#define SCANLINE_DOTS   456
#define OAM_SEARCH_END   79
#define VRAM_SEARCH_END 251
#define LINE_END        455
#define VISIBLE_LINES   144
#define TOTAL_LINES     154
#define OAM_SIZE 	160

static const uint32_t SCREEN_WIDTH  = 160;
static const uint32_t SCREEN_HEIGHT = 144;
static const uint32_t PIXELS_NUMBER = SCREEN_WIDTH*SCREEN_HEIGHT;


bool ppu_init(Ppu* ppu) {

	ppu->dots = 0;
	ppu->pixels = calloc((size_t) PIXELS_NUMBER, sizeof *ppu->pixels);

	return true;
}

static bool line_rendered = false;

static uint32_t bw_palette[4] = {
    0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000
};

static inline uint32_t palette_color(uint8_t shade) {
    return bw_palette[shade & 3];
}

static inline uint8_t set_stat_mode(uint8_t stat, uint8_t mode) {
	return ((stat & (uint8_t) ~0x03) | (mode & 0x03));
}

static inline uint8_t shade_from_palette(uint8_t palette, uint8_t color_index) {
	return (uint8_t) ((palette >> (color_index * 2u)) & 0x03);
}

static inline uint16_t tile_data_addr(uint8_t lcdc, uint8_t tile_index) {
	if (lcdc & (1 << 4)) return (uint16_t) (0x8000 + (uint16_t) tile_index * 16);
    	return (uint16_t) (0x9000 + (int16_t) ((int8_t) tile_index * 16));
}

static uint8_t pixel_index(Bus* bus, uint16_t base, uint16_t sx, uint16_t sy) {

	uint8_t lcdc = bus_read(bus, REG_LCDC);

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

void line_render(Ppu* ppu, Bus* bus, uint8_t ly) {

	uint8_t lcdc = bus_read(bus, REG_LCDC);
	uint8_t bgp  = bus_read(bus, REG_BGP);

	uint8_t line_bg_indices[SCREEN_WIDTH];

	// for background
	uint8_t scx = bus_read(bus, REG_SCX);
	uint8_t scy = bus_read(bus, REG_SCY);
	uint16_t base_b = (lcdc & (1 << 3)) ? 0x9C00 : 0x9800;

	// for window
	uint8_t wy = bus_read(bus, REG_WY);
	uint8_t wx = bus_read(bus, REG_WX);
	int16_t wx_origin = (int16_t) wx - 7;
	uint16_t base_w = (lcdc & (1 << 6)) ? 0x9C00 : 0x9800;

	bool w = (lcdc & LCDC_WINDOW_ENABLE) && (ly >= wy) && (wx_origin < 160);
	for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {

		uint8_t color_index = 0;

		if (lcdc & LCDC_BACKGROUND_ENABLE) {
			color_index = pixel_index(bus, base_b, (uint16_t) (x + scx), (uint16_t) (ly + scy));
		}

		// override bg
        	if (w && (int16_t) x >= wx_origin) {
			color_index = pixel_index(bus, base_w, (uint16_t) ((int16_t) x - wx_origin), ppu->window_line);
        	}
		line_bg_indices[x] = color_index;
        	ppu->pixels[x + ly * SCREEN_WIDTH] = palette_color(shade_from_palette(bgp, color_index));
    	}
	if (w) ppu->window_line++;

	// sprites disabled
	if (!(lcdc & LCDC_SPRITE_ENABLE)) {
		line_rendered = true;
		return;
	}

	// render sprites
	const uint8_t sprite_h = (lcdc & LCDC_SPRITE_SIZE) ? 16 : 8;
	const uint8_t* oam = bus->oa_ram;

	typedef struct {
		int16_t sy;
		int16_t sx;
		uint8_t tile;
		uint8_t flags;
	} SpriteInfo;

	SpriteInfo visible[10];
	uint32_t visible_count = 0;

	// collect up to 10 visible sprites in OAM order
	for (uint32_t sprite = 0; sprite < 40 && visible_count < 10; sprite++) {
		const uint8_t* entry = oam + (sprite * 4);
		int16_t sy = (int16_t) entry[0] - 16;
		int16_t sx = (int16_t) entry[1] - 8;

		if ((int16_t) ly < sy || (int16_t) ly >= (sy + (int16_t) sprite_h)) {
			continue;
		}
		visible[visible_count++] = (SpriteInfo) { sy, sx, entry[2], entry[3] };
	}

	// sort by x ascending, OAM order preserved on ties
	for (uint32_t i = 0; i < visible_count; i++) {
	for (uint32_t j = i + 1; j < visible_count; j++) {
		if (visible[j].sx < visible[i].sx) {
			SpriteInfo tmp = visible[i];
			visible[i] = visible[j];
			visible[j] = tmp;
		}
	}}

	// draw in reverse OAM order so lower OAM index wins
	for (int32_t s = (int32_t) visible_count - 1; s >= 0; s--) {
		SpriteInfo sp = visible[s];

		uint8_t tile = sp.tile;
		uint8_t flags = sp.flags;

		uint8_t row = (uint8_t) ((int16_t) ly - sp.sy);

		if (flags & 0x40) {
			row = (uint8_t) ((sprite_h - 1) - row);
		}

		if (sprite_h == 16) {
			tile &= 0xFE;
			if (row >= 8) {
				tile = (uint8_t) (tile + 1);
				row  = (uint8_t) (row - 8);
			}
		}

		uint16_t tile_addr = (uint16_t) (0x8000 + (uint16_t) tile * 16);
		uint8_t low  = bus_read(bus, (uint16_t) (tile_addr + (uint16_t) (row * 2)));
		uint8_t high = bus_read(bus, (uint16_t) (tile_addr + (uint16_t) (row * 2) + 1));

		uint8_t palette = (flags & 0x10) ? bus_read(bus, REG_OBP1) : bus_read(bus, REG_OBP0);
		bool behind_bg = (flags & 0x80) != 0;
		bool xflip = (flags & 0x20) != 0;

		for (uint32_t px = 0; px < 8; px++) {
			int16_t screen_x = sp.sx + (int16_t) px;
			if (screen_x < 0 || screen_x >= (int16_t) SCREEN_WIDTH) {
				continue;
			}

			uint8_t bit = xflip ? (uint8_t) px : (uint8_t) (7 - px);
			uint8_t color_index = (uint8_t) ((((high >> bit) & 1u) << 1u) | ((low >> bit) & 1u));
			if (color_index == 0) {
				continue;
			}

			if (behind_bg && line_bg_indices[screen_x] != 0) {
				continue;
			}

			uint8_t shade = shade_from_palette(palette, color_index);
			ppu->pixels[(ly * (uint32_t) SCREEN_WIDTH) + (uint32_t) screen_x] = palette_color(shade);
		}
	}
	line_rendered = true;
}

void ppu_step(Ppu* ppu, Bus* bus, EXTBackendContext* ctx, uint32_t cycles) {

	uint8_t lcdc = bus_read(bus, REG_LCDC);

	if (!(lcdc & LCDC_ENABLE)) {
		ppu->dots = 0;
		ppu->window_line = 0;
		bus_write(bus, REG_LY, 0x00);
		return;
	}

	ppu->dots += cycles;

	// handle OAM DMA transfers if ongoing
	uint8_t dma = bus_read(bus, REG_DMA);
	if (dma != 0) {
		uint16_t dma_src_addr = dma << 8;
		for (size_t i = 0; i < OAM_SIZE; i++) {
			uint8_t dma_src = bus_read(bus, dma_src_addr + (uint16_t) i);
			bus_write(bus, 0xFE00 + (uint16_t) i, dma_src);
		}
		bus_write(bus, REG_DMA, 0x00);
	}

	uint8_t mode = bus_read(bus, REG_STAT) & 0x03;

	// local hw regs, write back later
	uint8_t ly = bus_read(bus, REG_LY);
	uint8_t lyc = bus_read(bus, REG_LYC);
	uint8_t stat = bus_read(bus, REG_STAT);

	if (mode == 1) {
		if (ppu->dots >= SCANLINE_DOTS) {
			ppu->dots -= SCANLINE_DOTS;
			ly += 1;
			line_rendered = false;
			if (ly >= TOTAL_LINES) {
				stat = set_stat_mode(stat, MODE_OAM);
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
		bus_write(bus, REG_LY  , ly);
		bus_write(bus, REG_STAT, stat);
		return;
	}

	if (ppu->dots <= OAM_SEARCH_END) {
		// mode 2, read OAM
		stat = set_stat_mode(stat, MODE_OAM);

		if (mode != 2 && (stat & 0x20)) interrupt_send(bus, 1);

	} else if (ppu->dots <= VRAM_SEARCH_END) {
		// mode 3, read OAM and VRAM
		stat = set_stat_mode(stat, MODE_VRAM);

		if (!line_rendered) line_render(ppu, bus, ly);

	} else if (ppu->dots <= LINE_END) {
		// mode 0, HBlank
		stat = set_stat_mode(stat, MODE_HBLANK);

		if (mode != 0 && (stat & 0x08)) interrupt_send(bus, 1);

	} else {
		// horizontal line complete
		ppu->dots -= SCANLINE_DOTS;
		ly += 1;
		line_rendered = false;

		if (ly >= VISIBLE_LINES) {
			stat = set_stat_mode(stat, MODE_VBLANK);
			interrupt_send(bus, 0);
			EXT_backend_present(ctx);
		} else {
			stat = set_stat_mode(stat, MODE_OAM);
			if (mode != 2 && (stat & 0x20)) interrupt_send(bus, 1);
		}

		if (ly == lyc) {
			stat |= 0x04;
			if (stat & 0x40) interrupt_send(bus, 1);
		} else {
			stat &= (uint8_t) ~0x04;
		}
	}
	bus_write(bus, REG_LY  , ly);
	bus_write(bus, REG_STAT, stat);
}

