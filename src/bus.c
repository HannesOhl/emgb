#include "../inc/bus.h"
#include "../inc/util.h"

#include <stdlib.h>

#define MAX_ROM_SIZE 8000000

static uint8_t joypad_read(Bus* bus) {

	return 0xFF;
}

uint8_t bus_read(Bus* bus, uint16_t addr) {

	// read from boot rom
	if (bus->b_enabled && addr < 0x0100) {
			   return bus-> b_rom[addr];
	}

	if (addr < 0x8000) return bus-> c_rom[addr];

	if (addr == IO_P1) return joypad_read(bus);

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
	if (addr == 0xFF50 && val != 0) { bus->b_enabled = false; return; }

	if (addr < 0x8000) return;

	if (addr < 0xA000) { bus-> v_ram[addr - 0x8000] = val;  return; }
	if (addr < 0xC000) { bus-> c_ram[addr - 0xA000] = val;  return; }
	if (addr < 0xE000) { bus-> w_ram[addr - 0xC000] = val;  return; }
	if (addr < 0xFE00) { bus-> w_ram[addr - 0xE000] = val;  return; }
	if (addr < 0xFEA0) { bus->oa_ram[addr - 0xFE00] = val;  return; }
	if (addr < 0xFF00) { return; }

	if (addr == 0xFF46) {
		uint16_t src = (uint16_t)val << 8;
		for (int i = 0; i < 0xA0; i++) {
			bus->oa_ram[i] = bus_read(bus, (uint16_t)(src + i));
		}
		return;
	}

	if (addr < 0xFF80) { bus->io_ram[addr - 0xFF00] = val;  return; }
	bus->hi_ram[addr - 0xFF80] = val;
}

void bus_init(Bus* bus, FILE* rom_b, FILE* rom) {

	bus->c_rom = calloc((size_t) MAX_ROM_SIZE, sizeof *bus->c_rom);
	if (!bus->c_rom) {
		die("Error allocating busory for ROM.\n");
	}

	size_t ret = fread(bus->c_rom, 1, MAX_ROM_SIZE, rom);
	ret = fread(bus->b_rom, 1, 256, rom_b);
	(void) ret;

	bus->b_enabled = true;

	bus->joyp_select  = 0x30;
	bus->joyp_dpad    = 0x0F;
	bus->joyp_buttons = 0x0F;

	bus_write(bus, IO_P1, 0x0F);

	bus_write(bus, IO_LCDC, 0x91);
	bus_write(bus, IO_BGP , 0xFC);
	bus_write(bus, IO_OBP0, 0xFF);
	bus_write(bus, IO_OBP1, 0xFF);
}

