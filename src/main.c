#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define MAX_ROM_SIZE 8000000

static char* pname;

static inline void die(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	if (fmt) vfprintf(stderr, fmt, ap);
	if (!fmt) fprintf(stderr, "Unknown error.\n");
	va_end(ap);

	if (fmt && fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}

	exit(EXIT_FAILURE);
}

typedef struct {
	union {
		struct {
			uint8_t f;
			uint8_t a;
		};
		uint16_t af;
	};
	union {
		struct {
			uint8_t c;
			uint8_t b;
		};
		uint16_t bc;
	};
	union {
		struct {
			uint8_t e;
			uint8_t d;
		};
		uint16_t de;
	};
	union {
		struct {
			uint8_t l;
			uint8_t h;
		};
		uint16_t hl;
	};
	uint16_t sp;
	uint16_t pc;
} Registers;
Registers reg;

typedef enum {
	FLAG_C = (1 << 4),
	FLAG_H = (1 << 5),
	FLAG_N = (1 << 6),
	FLAG_Z = (1 << 7)
} Flag;

void flag_set(Flag f) {
	reg.f |= (uint8_t) f;
}
void flag_clear(Flag f) {
	reg.f &= (uint8_t) ~f;
}
void flag_cond(Flag f, bool cond) {
	reg.f = cond ? (reg.f | (uint8_t) f) : (reg.f & (uint8_t) ~f);
}

typedef struct {
	uint8_t* c_rom; 		// 0x0000 - 0x7FFF  32 kB
	uint8_t v_ram[8192]; 		// 0x8000 - 0x9FFF   8 kB
	uint8_t c_ram[8192];		// 0xA000 - 0xBFFF   8 kB
	uint8_t w_ram[8192];  		// 0xC000 - 0xDFFF   8 kB
//             unused, echoes w_ram        0xE000 - 0xFDFF 7.5 kB
	uint8_t oa_ram[160];		// 0xFE00 - 0xFE9F 160 B
//             unused                      0xFEA0 - 0xFEFF  96 B
	uint8_t io_ram[128];  		// 0xFF00 - 0xFF7F 128 B
	uint8_t hi_ram[128];		// 0xFF80 - 0xFFFF 128 B
} Memory;

uint16_t u16(uint8_t lsb, uint8_t msb) {
	return lsb | (msb << 8);
}

uint8_t lsb_make(uint16_t value) {
	return (uint8_t) (value & 0x00FF);
}

uint8_t msb_make(uint16_t value) {
	return (uint8_t) (value >> 8);
}

uint8_t bus_read(Memory* mem, uint16_t addr) {

	if (addr < 0x8000) return mem-> c_rom[addr];

	if (addr < 0xA000) return mem-> v_ram[addr - 0x8000];
	if (addr < 0xC000) return mem-> c_ram[addr - 0xA000];
	if (addr < 0xE000) return mem-> w_ram[addr - 0xC000];
	if (addr < 0xFE00) return mem-> w_ram[addr - 0xE000];
	if (addr < 0xFEA0) return mem->oa_ram[addr - 0xFE00];
	if (addr < 0xFF00) return 0xFF;
	if (addr < 0xFF80) return mem->io_ram[addr - 0xFF00];

	return mem->hi_ram[addr - 0xFF80];
}


void bus_write(Memory* mem, uint16_t addr, uint8_t val) {

	if (addr < 0x8000) return;

	if (addr < 0xA000) { mem-> v_ram[addr - 0x8000] = val;  return; }
	if (addr < 0xC000) { mem-> c_ram[addr - 0xA000] = val;  return; }
	if (addr < 0xE000) { mem-> w_ram[addr - 0xC000] = val;  return; }
	if (addr < 0xFE00) { mem-> w_ram[addr - 0xE000] = val;  return; }
	if (addr < 0xFEA0) { mem->oa_ram[addr - 0xFE00] = val;  return; }
	if (addr < 0xFF00) { return; }
	if (addr < 0xFF80) { mem->io_ram[addr - 0xFF00] = val;  return; }

	mem->hi_ram[addr - 0xFF80] = val;
}

void stack_print(Memory* mem) {

	printf("stack (SP=0x%04X):\n", reg.sp);

	uint16_t addr = reg.sp;
	while (addr + 1 <= 0xFFFE) {
		uint8_t lsb = bus_read(mem, addr++);
		uint8_t msb = bus_read(mem, addr++);
		uint16_t word = u16(lsb, msb);

		printf("\t0x%04X: 0x%04X", addr, word);
	}
	printf("\n");
}

void cpu_state_print() {

	printf("cpu state:\n");
	printf("A = %02X, F = %02X\n", reg.a, reg.f);
	printf("B = %02X, C = %02X\n", reg.b, reg.c);
	printf("D = %02X, E = %02X\n", reg.d, reg.e);
	printf("H = %02X, L = %02X\n", reg.h, reg.l);
	printf("\n");
}

uint32_t cpu_step(Memory* mem) {

	uint8_t opcode = bus_read(mem, reg.pc++);

	switch (opcode) {

	// CALL nn
	case 0xCD: {
		uint8_t lsb = bus_read(mem, reg.pc++);
		uint8_t msb = bus_read(mem, reg.pc++);
		uint16_t nn = u16(lsb, msb);
		bus_write(mem, --reg.sp, msb_make(reg.pc));
		bus_write(mem, --reg.sp, lsb_make(reg.pc));
		reg.pc = nn;
		return 24;
	} break;

	// LDH A, (n)
	case 0xF0: {
		uint8_t n = bus_read(mem, reg.pc++);
		reg.a = bus_read(mem, u16(n, 0xFF));
		return 12;
	} break;

	// LDH (n), A
	case 0xE0: {
		uint8_t n = bus_read(mem, reg.pc++);
		bus_write(mem, u16(n, 0xFF), reg.a);
		return 12;
	} break;

	// R
	case 0xCB: {
		opcode = bus_read(mem, reg.pc++);
		switch (opcode) {
		// TODO:
		// CB opcodes are highly structured
		// 	-> write dispatch function

		// RES 0, A
		case 0x87: {
			reg.a &= (uint8_t) ~(1 << 0);
			return 4;
		} break;
		default:
			die("Instruction 0xCB 0x%hhX not implemented.\n", opcode);
			return 0;
			break;
		}
	} break;

	// CP n
	case 0xFE: {
		uint8_t n = bus_read(mem, reg.pc++);
		uint8_t r = reg.a - n;
		bool c = reg.a < n;
		bool hc = (reg.a & 0x0F) < (n & 0x0F);
		flag_cond(FLAG_Z, !r);
		flag_set(FLAG_N);
		flag_cond(FLAG_H, hc);
		flag_cond(FLAG_C, c);
		return 12;
	} break;

	default:
		die("Instruction 0x%hhX not implemented.\n", opcode);
		return 0;
		break;
	}
}

int main(int argc, char** argv) {

	pname = argv[0];
	if (argc < 2) {
		die("Usage: %s rom.gb\n", pname);
	}

	Memory mem = {0};
	mem.c_rom = calloc((size_t) MAX_ROM_SIZE, sizeof *mem.c_rom);
	if (!mem.c_rom) {
		die("Error allocating memory for ROM.\n");
	}

	FILE* rom = fopen(argv[1], "rb");
	if (!rom) {
		die("Error opening ROM.\n");
	}

	size_t ret = fread(mem.c_rom, 1, MAX_ROM_SIZE, rom);
	(void) ret;

	reg.pc = 0x0150;
	reg.sp = 0xFFFE;


	while (true) {
		uint32_t cycles = cpu_step(&mem);
		stack_print(&mem);
		cpu_state_print();
	}

	free(mem.c_rom);
	fclose(rom);
	return 0;
}

