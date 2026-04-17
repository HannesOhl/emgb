#include "../inc/bus.h"

#include <stdint.h>

uint8_t bus_read(Bus* bus, uint16_t addr) {

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


void bus_write(Bus* bus, uint16_t addr, uint8_t val) {

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

