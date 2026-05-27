#ifndef BUS_H
#define BUS_H

#include <stdio.h>
#include <stdint.h>

#define REG_P1   0xFF00
#define REG_DIV  0xFF04
#define REG_TIMA 0xFF05
#define REG_TMA  0xFF06
#define REG_TAC  0xFF07
#define REG_IF   0xFF0F

#define REG_LCDC 0xFF40
#define REG_STAT 0xFF41
#define REG_SCY  0xFF42
#define REG_SCX  0xFF43
#define REG_LY   0xFF44
#define REG_LYC  0xFF45
#define REG_DMA  0xFF46
#define REG_BGP  0xFF47
#define REG_OBP0 0xFF48
#define REG_OBP1 0xFF49
#define REG_BDIS 0xFF50
#define REG_WY   0xFF4A
#define REG_WX   0xFF4B

#define REG_IE	 0xFFFF

typedef struct {

	bool b_enabled;
	uint8_t b_rom[256]; // 0x0000 - 0x00FF 256 B

	uint8_t c_rom[32768];	// 0x0000 - 0x7FFF  32 kB
	uint8_t v_ram[8192];	// 0x8000 - 0x9FFF   8 kB
	uint8_t c_ram[8192];	// 0xA000 - 0xBFFF   8 kB
	uint8_t w_ram[8192];	// 0xC000 - 0xDFFF   8 kB
// unused, echoes w_ram   	   0xE000 - 0xFDFF 7.5 kB
	uint8_t oa_ram[160]; 	// 0xFE00 - 0xFE9F 160 B
// unused                          0xFEA0 - 0xFEFF  96 B
	uint8_t io_ram[128]; 	// 0xFF00 - 0xFF7F 128 B
	uint8_t hi_ram[128]; 	// 0xFF80 - 0xFFFF 128 B

	uint8_t input_state;
} Bus;

void interrupt_send(Bus* bus, uint8_t n);
bool bus_init(Bus* bus, FILE* rom_boot, FILE* rom_game);
uint8_t bus_read(Bus* bus, uint16_t addr);
void bus_write(Bus* bus, uint16_t addr, uint8_t val);

#endif

