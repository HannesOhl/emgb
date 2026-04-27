#include "../inc/ppu.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum { FB_WIDTH = 160, FB_HEIGHT = 144 };

static uint32_t dmg_palette[4] = {
    0xFFFFFFFFu,
    0xFFAAAAAAu,
    0xFF555555u,
    0xFF000000u,
};

static inline uint32_t palette_color(uint8_t shade) {
    return dmg_palette[shade & 3u];
}

static inline uint8_t shade_from_palette(uint8_t palette, uint8_t color_index) {
    return (uint8_t)((palette >> (color_index * 2u)) & 0x03u);
}

static inline uint8_t tile_pixel_index(Ppu *ppu, uint16_t tile_addr, uint8_t row, uint8_t col) {
    uint8_t low = bus_read(ppu->bus, (uint16_t)(tile_addr + (uint16_t)(row * 2u)));
    uint8_t high = bus_read(ppu->bus, (uint16_t)(tile_addr + (uint16_t)(row * 2u) + 1u));
    uint8_t bit = (uint8_t)(7u - (col & 7u));
    return (uint8_t)((((high >> bit) & 1u) << 1u) | ((low >> bit) & 1u));
}

static uint8_t background_pixel_index(Ppu *ppu, uint8_t x, uint8_t y) {
    uint8_t scx = bus_read(ppu->bus, IO_SCX);
    uint8_t scy = bus_read(ppu->bus, IO_SCY);
    uint8_t lcdc = bus_read(ppu->bus, IO_LCDC);

    uint8_t gx = (uint8_t)(x + scx);
    uint8_t gy = (uint8_t)(y + scy);

    uint16_t map_base = (lcdc & (1u << 3)) ? 0x9C00u : 0x9800u;
    uint16_t tile_x = (uint16_t)(gx >> 3);
    uint16_t tile_y = (uint16_t)(gy >> 3);
    uint16_t map_offset = (uint16_t)(tile_y * 32u + tile_x);
    uint8_t tile_index = bus_read(ppu->bus, (uint16_t)(map_base + map_offset));

    uint16_t tile_addr;
    if (lcdc & (1u << 4)) {
        tile_addr = (uint16_t)(0x8000u + (uint16_t)tile_index * 16u);
    } else {
        int8_t signed_index = (int8_t)tile_index;
        tile_addr = (uint16_t)(0x9000u + (int16_t)signed_index * 16);
    }

    return tile_pixel_index(ppu, tile_addr, (uint8_t)(gy & 7u), (uint8_t)(gx & 7u));
}

static uint8_t window_pixel_index(Ppu *ppu, uint8_t x, uint8_t y, uint8_t wx_origin, uint8_t wy) {
    uint8_t lcdc = bus_read(ppu->bus, IO_LCDC);

    uint8_t wx = (uint8_t)(x - wx_origin);
    uint8_t wy_local = (uint8_t)(y - wy);

    uint16_t map_base = (lcdc & (1u << 6)) ? 0x9C00u : 0x9800u;
    uint16_t tile_x = (uint16_t)(wx >> 3);
    uint16_t tile_y = (uint16_t)(wy_local >> 3);
    uint16_t map_offset = (uint16_t)(tile_y * 32u + tile_x);
    uint8_t tile_index = bus_read(ppu->bus, (uint16_t)(map_base + map_offset));

    uint16_t tile_addr;
    if (lcdc & (1u << 4)) {
        tile_addr = (uint16_t)(0x8000u + (uint16_t)tile_index * 16u);
    } else {
        int8_t signed_index = (int8_t)tile_index;
        tile_addr = (uint16_t)(0x9000u + (int16_t)signed_index * 16);
    }

    return tile_pixel_index(ppu, tile_addr, (uint8_t)(wy_local & 7u), (uint8_t)(wx & 7u));
}

static void render_scanline(Ppu *ppu) {
    uint8_t lcdc = bus_read(ppu->bus, IO_LCDC);
    uint8_t bgp = bus_read(ppu->bus, IO_BGP);
    uint8_t wy = bus_read(ppu->bus, IO_WY);
    uint8_t wx = bus_read(ppu->bus, IO_WX);
    uint8_t wx_origin = (wx <= 7u) ? 0u : (uint8_t)(wx - 7u);

    uint8_t line_bg_indices[FB_WIDTH];

    for (uint32_t x = 0; x < FB_WIDTH; ++x) {
        uint8_t color_index = 0u;

        if (lcdc & 0x01u) {
            color_index = background_pixel_index(ppu, (uint8_t)x, ppu->ly);

            if ((lcdc & 0x20u) && ppu->ly >= wy && x >= wx_origin) {
                color_index = window_pixel_index(ppu, (uint8_t)x, ppu->ly, wx_origin, wy);
            }
        }

        line_bg_indices[x] = color_index;
        ppu->pixels[(ppu->ly * FB_WIDTH) + x] = palette_color(shade_from_palette(bgp, color_index));
    }

    if (!(lcdc & 0x02u)) {
        return;
    }

    const uint8_t sprite_h = (lcdc & 0x04u) ? 16u : 8u;
    const uint8_t *oam = ppu->bus->oa_ram;

    for (uint32_t sprite = 0; sprite < 40u; ++sprite) {
        const uint8_t *entry = oam + (sprite * 4u);
        int sy = (int)entry[0] - 16;
        int sx = (int)entry[1] - 8;

        if ((int)ppu->ly < sy || (int)ppu->ly >= (sy + (int)sprite_h)) {
            continue;
        }

        uint8_t tile = entry[2];
        uint8_t flags = entry[3];
        uint8_t row = (uint8_t)((int)ppu->ly - sy);

        if (flags & 0x40u) {
            row = (uint8_t)((sprite_h - 1u) - row);
        }

        if (sprite_h == 16u) {
            tile &= 0xFEu;
            if (row >= 8u) {
                tile = (uint8_t)(tile + 1u);
                row = (uint8_t)(row - 8u);
            }
        }

        uint16_t tile_addr = (uint16_t)(0x8000u + (uint16_t)tile * 16u);
        uint8_t low = bus_read(ppu->bus, (uint16_t)(tile_addr + (uint16_t)(row * 2u)));
        uint8_t high = bus_read(ppu->bus, (uint16_t)(tile_addr + (uint16_t)(row * 2u) + 1u));

        uint8_t palette = (flags & 0x10u) ? bus_read(ppu->bus, IO_OBP1) : bus_read(ppu->bus, IO_OBP0);
        bool behind_bg = (flags & 0x80u) != 0;
        bool xflip = (flags & 0x20u) != 0;

        for (uint32_t px = 0; px < 8u; ++px) {
            int screen_x = sx + (int)px;
            if (screen_x < 0 || screen_x >= (int)FB_WIDTH) {
                continue;
            }

            uint8_t bit = xflip ? (uint8_t)px : (uint8_t)(7u - px);
            uint8_t color_index = (uint8_t)((((high >> bit) & 1u) << 1u) | ((low >> bit) & 1u));
            if (color_index == 0u) {
                continue;
            }

            if (behind_bg && line_bg_indices[screen_x] != 0u) {
                continue;
            }

            uint8_t shade = shade_from_palette(palette, color_index);
            ppu->pixels[(ppu->ly * FB_WIDTH) + (uint32_t)screen_x] = palette_color(shade);
        }
    }
}

void ppu_init(Ppu *ppu, Bus *bus, uint32_t *buffer) {
    ppu->bus = bus;
    ppu->pixels = buffer;
    ppu->dots = 0;
    ppu->ly = 0;

    if (ppu->pixels) {
        for (size_t i = 0; i < (size_t)FB_WIDTH * (size_t)FB_HEIGHT; ++i) {
            ppu->pixels[i] = 0xFFFFFFFFu;
        }
    }

    bus_write(ppu->bus, IO_LY, 0);
}

void ppu_step(Ppu *ppu, SDLContext *ctx, uint32_t cycles) {
    uint8_t lcdc = bus_read(ppu->bus, IO_LCDC);

    if (!(lcdc & 0x80u)) {
        ppu->dots = 0;
        ppu->ly = 0;
        bus_write(ppu->bus, IO_LY, 0);
        return;
    }

    ppu->dots += cycles;

    while (ppu->dots >= 456u) {
        ppu->dots -= 456u;

        if (ppu->ly < 144u) {
            render_scanline(ppu);
        }

        ppu->ly = (uint8_t)(ppu->ly + 1u);
        bus_write(ppu->bus, IO_LY, ppu->ly);

        if (ppu->ly == 144u) {
            bus_write(ppu->bus, IO_IF, (uint8_t)(bus_read(ppu->bus, IO_IF) | 0x01u));
            if (ctx && ctx->window) {
                SDL_UpdateWindowSurface(ctx->window);
            }
        }

        if (ppu->ly > 153u) {
            ppu->ly = 0;
            bus_write(ppu->bus, IO_LY, 0);
        }
    }
}
