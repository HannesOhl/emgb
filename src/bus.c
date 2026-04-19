#include "../inc/bus.h"
#include "../inc/util.h"

#include <stdlib.h>

#define MAX_ROM_SIZE 8000000

void bus_init(Bus* bus, FILE* rom) {

	bus->c_rom = calloc((size_t) MAX_ROM_SIZE, sizeof *bus->c_rom);
	if (!bus->c_rom) {
		die("Error allocating busory for ROM.\n");
	}

	size_t ret = fread(bus->c_rom, 1, MAX_ROM_SIZE, rom);
	(void) ret;
}

uint8_t bus_read(Bus* bus, uint16_t addr) {

	// read from boot rom
	if (bus->b_enabled && addr < 0x0100) {
			   return bus-> b_rom[addr];
	}

	if (addr < 0x8000) return bus-> c_rom[addr];

	if (addr < 0xA000) return bus-> v_ram[addr - 0x8000];
	if (addr < 0xC000) return bus-> c_ram[addr - 0xA000];
	if (addr < 0xE000) return bus-> w_ram[addr - 0xC000];
	if (addr < 0xFE00) return bus-> w_ram[addr - 0xE000];
	if (addr < 0xFEA0) return bus->oa_ram[addr - 0xFE00];
	if (addr < 0xFF00) return 0xFF;
	if (addr < 0xFF80) return bus->io_ram[addr - 0xFF00];

	return bus->hi_ram[addr - 0xFF80];
}

uint16_t bus_read_16(Bus* bus, uint16_t addr) {

	uint8_t lsb = bus_read(bus, addr);
	uint8_t msb = bus_read(bus, addr + 1);

	return u16(lsb, msb);
}

void bus_write(Bus* bus, uint16_t addr, uint8_t val) {

	// disable boot rom
	if (addr == 0xFF50 && val != 0) bus->b_enabled = false;

	if (addr < 0x8000) return;

	if (addr < 0xA000) { bus-> v_ram[addr - 0x8000] = val;  return; }
	if (addr < 0xC000) { bus-> c_ram[addr - 0xA000] = val;  return; }
	if (addr < 0xE000) { bus-> w_ram[addr - 0xC000] = val;  return; }
	if (addr < 0xFE00) { bus-> w_ram[addr - 0xE000] = val;  return; }
	if (addr < 0xFEA0) { bus->oa_ram[addr - 0xFE00] = val;  return; }
	if (addr < 0xFF00) { return; }
	if (addr < 0xFF80) { bus->io_ram[addr - 0xFF00] = val;  return; }

	bus->hi_ram[addr - 0xFF80] = val;
}

