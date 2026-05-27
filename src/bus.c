#include "../inc/bus.h"

#define MAX_ROM_SIZE 8000000
#define ROM_BOOT_SIZE 256

void interrupt_send(Bus* bus, uint8_t n) {
	uint8_t flag = bus_read(bus, REG_IF);
	flag |= (1 << n);
	bus_write(bus, REG_IF, flag);
}

static uint8_t joypad_read(Bus* bus) {

	uint8_t selection = bus->io_ram[REG_P1 - 0xFF00];
	uint8_t res = 0xC0 | (selection & 0x30) | 0x0F;

	if ((selection & 0x10) == 0) {
		res &= (0xF0 | (bus->input_state & 0x0F));
	}

	if ((selection & 0x20) == 0) {
		res &= (0xF0 | ((bus->input_state >> 4) & 0x0F));
	}

	return res;
}

static void joypad_write(Bus* bus, uint8_t val) {
	uint8_t res = bus_read(bus, REG_P1);
	res = (res & 0xCF) | (val & 0x30);
	bus->io_ram[REG_P1 - 0xFF00] = (res & 0xCF) | (val & 0x30);
}

uint8_t bus_read(Bus* bus, uint16_t addr) {

	// read from boot rom
	if (bus->b_enabled && addr < 0x0100) {
			   return bus-> b_rom[addr];
	}

	if (addr == REG_P1) return joypad_read(bus);

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

	// disable boot rom
	if (addr == REG_BDIS && val != 0) { bus->b_enabled = false; return; }

	if (addr == REG_P1) { joypad_write(bus, val); return;}

	if (addr == REG_DMA) {
		uint16_t src = (uint16_t) val << 8;
		for (int i = 0; i < 0xA0; i++) {
			bus->oa_ram[i] = bus_read(bus, (uint16_t) (src + i));
		}
		return;
	}

	if (addr < 0x8000) { 					return; }
	if (addr < 0xA000) { bus-> v_ram[addr - 0x8000] = val;  return; }
	if (addr < 0xC000) { bus-> c_ram[addr - 0xA000] = val;  return; }
	if (addr < 0xE000) { bus-> w_ram[addr - 0xC000] = val;  return; }
	if (addr < 0xFE00) { bus-> w_ram[addr - 0xE000] = val;  return; }
	if (addr < 0xFEA0) { bus->oa_ram[addr - 0xFE00] = val;  return; }
	if (addr < 0xFF00) { 					return; }
	if (addr < 0xFF80) { bus->io_ram[addr - 0xFF00] = val;  return; }

	bus->hi_ram[addr - 0xFF80] = val;
}

bool bus_init(Bus* bus, FILE* rom_boot, FILE* rom_game) {

	size_t ret = fread(bus->c_rom, 1, MAX_ROM_SIZE, rom_game);
	if (ret == 0) {
		fprintf(stderr, "[ERROR]: Reading the game-rom failed.\n");
		return false;
	}

	if (rom_boot) {
		ret = fread(bus->b_rom, 1, ROM_BOOT_SIZE, rom_boot);
		if (ret == ROM_BOOT_SIZE) {
			bus->b_enabled = true;
		} else {
			fprintf(stderr, "[INFO]: Reading the boot-rom failed. We skip it.\n");
			bus->b_enabled = false;
		}
	} else {
		bus->b_enabled = false;
	}

	bus->input_state = 0xFF;

	bus_write(bus, REG_P1  , 0x30);
	bus_write(bus, REG_LCDC, 0x91);
	bus_write(bus, REG_BGP , 0xFC);
	bus_write(bus, REG_OBP0, 0xFF);
	bus_write(bus, REG_OBP1, 0xFF);

	return true;
}

