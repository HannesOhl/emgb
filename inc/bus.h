#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include <stdio.h>

#define HW_REG_IE 0xFFFF
#define HW_REG_IF 0xFF0F

#define IO_P1   0xFF00
#define IO_DIV  0xFF04
#define IO_TIMA 0xFF05
#define IO_TMA  0xFF06
#define IO_TAC  0xFF07
#define IO_IF   0xFF0F

#define IO_LCDC 0xFF40
#define IO_STAT 0xFF41
#define IO_SCY  0xFF42
#define IO_SCX  0xFF43
#define IO_LY   0xFF44
#define IO_LYC  0xFF45
#define IO_DMA  0xFF46
#define IO_BGP  0xFF47
#define IO_OBP0 0xFF48
#define IO_OBP1 0xFF49
#define IO_WY   0xFF4A
#define IO_WX   0xFF4B
#define IO_IF2  0xFF0F

typedef struct {

	uint8_t b_rom[256];		// 0x0000 - 0x00FF 256 B
	bool b_enabled;

	uint8_t* c_rom; 		// 0x0000 - 0x7FFF  32 kB
	uint8_t v_ram[8192]; 		// 0x8000 - 0x9FFF   8 kB
	uint8_t c_ram[8192];		// 0xA000 - 0xBFFF   8 kB
	uint8_t w_ram[8192];  		// 0xC000 - 0xDFFF   8 kB
//             unused, echoes w_ram        0xE000 - 0xFDFF 7.5 kB
	uint8_t oa_ram[160];		// 0xFE00 - 0xFE9F 160 B
//             unused                      0xFEA0 - 0xFEFF  96 B
	uint8_t io_ram[128];  		// 0xFF00 - 0xFF7F 128 B
	uint8_t hi_ram[128];		// 0xFF80 - 0xFFFF 128 B

	uint8_t joyp_select;
	uint8_t joyp_dpad;
	uint8_t joyp_buttons;
} Bus;

void bus_init(Bus* bus, FILE* b_rom, FILE* rom);

uint8_t bus_read(Bus* bus, uint16_t addr);
uint16_t bus_read_16(Bus* bus, uint16_t addr);
void bus_write(Bus* bus, uint16_t addr, uint8_t val);

#endif

