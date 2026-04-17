#include "../inc/cpu.h"

#include "../inc/util.h"

void flag_set(Registers* reg, Flag f) {
	reg->f |= (uint8_t) f;
}
void flag_clear(Registers* reg, Flag f) {
	reg->f &= (uint8_t) ~f;
}
void flag_cond(Registers* reg, Flag f, bool cond) {
	reg->f = cond ? (reg->f | (uint8_t) f) : (reg->f & (uint8_t) ~f);
}

void stack_print(Cpu* cpu, Bus* bus) {

	Registers* reg = &cpu->reg;

	printf("stack (SP=0x%04X):\n", reg->sp);

	uint16_t addr = reg->sp;
	while (addr + 1 <= 0xFFFE) {
		uint8_t lsb = bus_read(bus, addr++);
		uint8_t msb = bus_read(bus, addr++);
		uint16_t word = u16(lsb, msb);

		printf("\t0x%04X: 0x%04X", addr, word);
	}
	printf("\n");
}

void cpu_state_print(Cpu* cpu) {

	Registers* reg = &cpu->reg;

	printf("cpu state:\n");
	printf("A = %02X, F = %02X\n", reg->a, reg->f);
	printf("B = %02X, C = %02X\n", reg->b, reg->c);
	printf("D = %02X, E = %02X\n", reg->d, reg->e);
	printf("H = %02X, L = %02X\n", reg->h, reg->l);
	printf("\n");
}

void cpu_init(Cpu* cpu) {

	cpu->reg.pc = 0x0150;
	cpu->reg.sp = 0xFFFE;
}

uint32_t cpu_step(Cpu* cpu, Bus* bus) {

	Registers* reg = &cpu->reg;

	uint8_t opcode = bus_read(bus, reg->pc++);

	switch (opcode) {

	// LD rr, nn
	case 0x01: reg->bc = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;
	case 0x11: reg->de = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;
	case 0x21: reg->hl = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;
	case 0x31: reg->sp = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;
	// LD (nn), SP

	// LD r, r'
	case 0x40: reg->b = reg->b; return 4;
	case 0x41: reg->b = reg->c; return 4;
	case 0x42: reg->b = reg->d; return 4;
	case 0x43: reg->b = reg->e; return 4;
	case 0x44: reg->b = reg->h; return 4;
	case 0x45: reg->b = reg->l; return 4;
	case 0x47: reg->b = reg->a; return 4;

	case 0x48: reg->c = reg->b; return 4;
	case 0x49: reg->c = reg->c; return 4;
	case 0x4A: reg->c = reg->d; return 4;
	case 0x4B: reg->c = reg->e; return 4;
	case 0x4C: reg->c = reg->h; return 4;
	case 0x4D: reg->c = reg->l; return 4;
	case 0x4F: reg->c = reg->a; return 4;

	case 0x50: reg->d = reg->b; return 4;
	case 0x51: reg->d = reg->c; return 4;
	case 0x52: reg->d = reg->d; return 4;
	case 0x53: reg->d = reg->e; return 4;
	case 0x54: reg->d = reg->h; return 4;
	case 0x55: reg->d = reg->l; return 4;
	case 0x57: reg->d = reg->a; return 4;

	case 0x58: reg->e = reg->b; return 4;
	case 0x59: reg->e = reg->c; return 4;
	case 0x5A: reg->e = reg->d; return 4;
	case 0x5B: reg->e = reg->e; return 4;
	case 0x5C: reg->e = reg->h; return 4;
	case 0x5D: reg->e = reg->l; return 4;
	case 0x5F: reg->e = reg->a; return 4;

	case 0x60: reg->h = reg->b; return 4;
	case 0x61: reg->h = reg->c; return 4;
	case 0x62: reg->h = reg->d; return 4;
	case 0x63: reg->h = reg->e; return 4;
	case 0x64: reg->h = reg->h; return 4;
	case 0x65: reg->h = reg->l; return 4;
	case 0x67: reg->h = reg->a; return 4;

	case 0x68: reg->l = reg->b; return 4;
	case 0x69: reg->l = reg->c; return 4;
	case 0x6A: reg->l = reg->d; return 4;
	case 0x6B: reg->l = reg->e; return 4;
	case 0x6C: reg->l = reg->h; return 4;
	case 0x6D: reg->l = reg->l; return 4;
	case 0x6F: reg->l = reg->a; return 4;

	case 0x78: reg->a = reg->b; return 4;
	case 0x79: reg->a = reg->c; return 4;
	case 0x7A: reg->a = reg->d; return 4;
	case 0x7B: reg->a = reg->e; return 4;
	case 0x7C: reg->a = reg->h; return 4;
	case 0x7D: reg->a = reg->l; return 4;
	case 0x7F: reg->a = reg->a; return 4;

	// CALL nn
	case 0xCD: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		uint16_t nn = u16(lsb, msb);
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = nn;
		return 24;
	}

	// LDH A, (n)
	case 0xF0: {
		uint8_t n = bus_read(bus, reg->pc++);
		reg->a = bus_read(bus, u16(n, 0xFF));
		return 12;
	}

	// LDH (n), A
	case 0xE0: {
		uint8_t n = bus_read(bus, reg->pc++);
		bus_write(bus, u16(n, 0xFF), reg->a);
		return 12;
	}

	// R
	case 0xCB: {
		opcode = bus_read(bus, reg->pc++);
		switch (opcode) {
		// TODO:
		// CB opcodes are highly structured
		// 	-> write dispatch function

		// RES 0, A
		case 0x87: {
			reg->a &= (uint8_t) ~(1 << 0);
			return 4;
		}
		default:
			die("Instruction 0xCB 0x%hhX not implemented.\n", opcode);
			return 0;
		}
	}

	// CP n
	case 0xFE: {
		uint8_t n = bus_read(bus, reg->pc++);
		uint8_t r = reg->a - n;
		bool c = reg->a < n;
		bool hc = (reg->a & 0x0F) < (n & 0x0F);
		flag_cond(reg, FLAG_Z, !r);
		flag_set(reg, FLAG_N);
		flag_cond(reg, FLAG_H, hc);
		flag_cond(reg, FLAG_C, c);
		return 12;
	}

	default:
		die("Instruction 0x%hhX not implemented.\n", opcode);
		return 0;
	}
}

