#include "../inc/cpu.h"

#include "../inc/util.h"

static void flag_set(Registers* reg, Flag f) {
	reg->f |= (uint8_t) f;
}
static void flag_clr(Registers* reg, Flag f) {
	reg->f &= (uint8_t) ~f;
}
static void flag_con(Registers* reg, Flag f, bool cond) {
	reg->f = cond ? (reg->f | (uint8_t) f) : (reg->f & (uint8_t) ~f);
}
static bool flag_check(Registers* reg, Flag f) {
	return ( (reg->f & (uint8_t) f) != 0 );
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
	printf("cycles = %lu\n", cpu->cycles);

	printf("A = %02X, F = %02X\n", reg->a, reg->f);
	printf("B = %02X, C = %02X\n", reg->b, reg->c);
	printf("D = %02X, E = %02X\n", reg->d, reg->e);
	printf("H = %02X, L = %02X\n", reg->h, reg->l);
	printf("SP = %04X\n", reg->sp);
	printf("\n");
}

void cpu_init(Cpu* cpu) {

	cpu->reg.pc = 0x0000;
}

static uint8_t cb_register_get(Bus* bus, Registers* reg, uint8_t r) {

	switch (r) {

	case 0: return reg->b;
	case 1: return reg->c;
	case 2: return reg->d;
	case 3: return reg->e;
	case 4: return reg->h;
	case 5: return reg->l;
	case 6: return bus_read(bus, reg->hl);
	case 7: return reg->a;

	default: die("invalid branch in r_get_execute\n!"); return 0;

	}
}

static void cb_register_set(Bus* bus, Registers* reg, uint8_t r, uint8_t val) {

	switch (r) {

	case 0: reg->b = val; return;
	case 1: reg->c = val; return;
	case 2: reg->d = val; return;
	case 3: reg->e = val; return;
	case 4: reg->h = val; return;
	case 5: reg->l = val; return;
	case 6: bus_write(bus, reg->hl, val); return;
	case 7: reg->a = val; return;

	default: die("invalid branch in r_get_execute\n!"); break;

	}
}

uint8_t rlc(Registers* reg, uint8_t r_val) {
	die("rlc.\n");
	(void) r_val; return 0; }
uint8_t rrc(Registers* reg, uint8_t r_val) {
	die("rrc.\n");
	(void) r_val; return 0; }
uint8_t rl (Registers* reg, uint8_t r_val) {
	uint8_t c_in   = flag_check(reg, FLAG_C) ? 1 : 0;
	uint8_t c_out  = r_val >> 7 & 1;
	uint8_t result = (r_val << 1) | c_in;
	flag_con(reg, FLAG_C, c_out);
	flag_con(reg, FLAG_Z, result == 0);
	flag_clr(reg, FLAG_N);
	flag_clr(reg, FLAG_H);
	return result;
}
uint8_t rr (Registers* reg, uint8_t r_val) {
	die("rr .\n");
	(void) r_val; return 0; }
uint8_t sla(Registers* reg, uint8_t r_val) {
	die("sla.\n");
	(void) r_val; return 0; }
uint8_t sra(Registers* reg, uint8_t r_val) {
	die("sra.\n");
	(void) r_val; return 0; }
uint8_t swp(Registers* reg, uint8_t r_val) {
	die("swp.\n");
	(void) r_val; return 0; }
uint8_t srl(Registers* reg, uint8_t r_val) {
	die("srl.\n");
	(void) r_val; return 0; }

// 0b xx xxx xxx
//     ^   ^   ^
//     |   |    - target register:
//     |   |	  0=b, 1=c, 2=d, 3=e, 4=h, 5=l, 6=(hl), 7=a
//     |    - 0b 00... operation selection
//     |      0b 01/10/11... bit index for BIT, RES, SET
//      - instruction group: 0b00: rot/shift, 0b01: BIT, 0b10: RES, 0b11: SET
static uint32_t cb_execute(Bus* bus, Registers* reg, uint8_t opcode) {

	uint8_t r = (opcode & 0b00000111) >> 0;
	uint8_t b = (opcode & 0b00111000) >> 3;
	uint8_t g = (opcode & 0b11000000) >> 6;

	uint32_t cycles = (r != 6) ? 8 : 16;
	uint8_t r_val = cb_register_get(bus, reg, r);

	switch (g) {

	// rotates and shifts
	case 0: {

		switch (b) {
		case 0: r_val = rlc(reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		case 1: r_val = rrc(reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		case 2: r_val = rl (reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		case 3: r_val = rr (reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		case 4: r_val = sla(reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		case 5: r_val = sra(reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		case 6: r_val = swp(reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		case 7: r_val = srl(reg, r_val); cb_register_set(bus, reg, r, r_val); break;
		default: die("invalid branch in cb_execute, case 0!\n");
		}
		return cycles;
	}

	// BIT
	case 1: {
		flag_con(reg, FLAG_Z, !(r_val & 1 << b));
		flag_clr(reg, FLAG_N);
		flag_set(reg, FLAG_H);
		return cycles;
	}

	// RES
	case 2: r_val &= (uint8_t) ~(1 << b); cb_register_set(bus, reg, r, r_val);
		return cycles;
	// SET
	case 3: r_val |= (uint8_t)  (1 << b); cb_register_set(bus, reg, r, r_val);
		return cycles;

	default:
		die("Instruction 0xCB 0x%hhX not implemented.\n", opcode);
		return 0;
	}
}

uint8_t inc_r(Registers* reg, uint8_t r) {

	uint8_t result = r + 1;
	flag_con(reg, FLAG_H, (r & 0x0F) == 0x0F);
	flag_con(reg, FLAG_Z, result == 0);
	flag_clr(reg, FLAG_N);

	return result;
}

uint32_t cpu_step(Cpu* cpu, Bus* bus) {

	Registers* reg = &cpu->reg;

	uint8_t opcode = bus_read(bus, reg->pc++);
	printf("executing opcode 0x%02X\n", opcode);
	switch (opcode) {

	// NOP 1/1
	case 0x00: return 4;

	// INC r
	case 0x04: reg->b = inc_r(reg, reg->b); return 4;
	case 0x14: reg->d = inc_r(reg, reg->d); return 4;
	case 0x24: reg->h = inc_r(reg, reg->h); return 4;
	case 0x0C: reg->c = inc_r(reg, reg->c); return 4;
	case 0x1C: reg->e = inc_r(reg, reg->e); return 4;
	case 0x2C: reg->l = inc_r(reg, reg->l); return 4;
	case 0x3C: reg->a = inc_r(reg, reg->a); return 4;

	// JR cc, e 2/2
	case 0x20: {
		int8_t e = (int8_t) bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_Z)) {
			reg->pc = (uint16_t) (reg->pc + (int16_t) e);
			return 12;
		}
		return 8;
	}
	case 0x30: {
		int8_t e = (int8_t) bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_C)) {
			reg->pc = (uint16_t) (reg->pc + (int16_t) e);
			return 12;
		}
		return 8;
	}


	// LD r, n 7/7
	case 0x06: reg->b = bus_read(bus, reg->pc++); return 8;
	case 0x16: reg->d = bus_read(bus, reg->pc++); return 8;
	case 0x26: reg->h = bus_read(bus, reg->pc++); return 8;
	case 0x0E: reg->c = bus_read(bus, reg->pc++); return 8;
	case 0x1E: reg->e = bus_read(bus, reg->pc++); return 8;
	case 0x2E: reg->l = bus_read(bus, reg->pc++); return 8;
	case 0x3E: reg->a = bus_read(bus, reg->pc++); return 8;

	// LD A, (BC)
	case 0x0A: reg->a = bus_read(bus, reg->bc); return 8;
	case 0x1A: reg->a = bus_read(bus, reg->de); return 8;

	// LD (HL), n 1/1
	case 0x36: {
		uint8_t n = bus_read(bus, reg->pc++);
		bus_write(bus, reg->hl, n);
		return 12;
	}

	// LD rr, nn 4/4
	case 0x01: reg->bc = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;
	case 0x11: reg->de = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;
	case 0x21: reg->hl = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;
	case 0x31: reg->sp = bus_read_16(bus, reg->pc); reg->pc += 2; return 12;

	// LD (HL+), A
	case 0x22: bus_write(bus, reg->hl++, reg->a); return 8;
	// LD (HL-), A
	case 0x32: bus_write(bus, reg->hl--, reg->a); return 8;
	// LD A, (HL+)
	case 0x2A: reg->a = bus_read(bus, reg->hl++); return 8;
	// LD A, (HL-)
	case 0x3A: reg->a = bus_read(bus, reg->hl--); return 8;

	// LD r, r' 49/49
	// b
	case 0x40: reg->b = reg->b; return 4;
	case 0x41: reg->b = reg->c; return 4;
	case 0x42: reg->b = reg->d; return 4;
	case 0x43: reg->b = reg->e; return 4;
	case 0x44: reg->b = reg->h; return 4;
	case 0x45: reg->b = reg->l; return 4;
	case 0x47: reg->b = reg->a; return 4;
	// c
	case 0x48: reg->c = reg->b; return 4;
	case 0x49: reg->c = reg->c; return 4;
	case 0x4A: reg->c = reg->d; return 4;
	case 0x4B: reg->c = reg->e; return 4;
	case 0x4C: reg->c = reg->h; return 4;
	case 0x4D: reg->c = reg->l; return 4;
	case 0x4F: reg->c = reg->a; return 4;
	// d
	case 0x50: reg->d = reg->b; return 4;
	case 0x51: reg->d = reg->c; return 4;
	case 0x52: reg->d = reg->d; return 4;
	case 0x53: reg->d = reg->e; return 4;
	case 0x54: reg->d = reg->h; return 4;
	case 0x55: reg->d = reg->l; return 4;
	case 0x57: reg->d = reg->a; return 4;
	// e
	case 0x58: reg->e = reg->b; return 4;
	case 0x59: reg->e = reg->c; return 4;
	case 0x5A: reg->e = reg->d; return 4;
	case 0x5B: reg->e = reg->e; return 4;
	case 0x5C: reg->e = reg->h; return 4;
	case 0x5D: reg->e = reg->l; return 4;
	case 0x5F: reg->e = reg->a; return 4;
	// h
	case 0x60: reg->h = reg->b; return 4;
	case 0x61: reg->h = reg->c; return 4;
	case 0x62: reg->h = reg->d; return 4;
	case 0x63: reg->h = reg->e; return 4;
	case 0x64: reg->h = reg->h; return 4;
	case 0x65: reg->h = reg->l; return 4;
	case 0x67: reg->h = reg->a; return 4;
	// l
	case 0x68: reg->l = reg->b; return 4;
	case 0x69: reg->l = reg->c; return 4;
	case 0x6A: reg->l = reg->d; return 4;
	case 0x6B: reg->l = reg->e; return 4;
	case 0x6C: reg->l = reg->h; return 4;
	case 0x6D: reg->l = reg->l; return 4;
	case 0x6F: reg->l = reg->a; return 4;
	// a
	case 0x78: reg->a = reg->b; return 4;
	case 0x79: reg->a = reg->c; return 4;
	case 0x7A: reg->a = reg->d; return 4;
	case 0x7B: reg->a = reg->e; return 4;
	case 0x7C: reg->a = reg->h; return 4;
	case 0x7D: reg->a = reg->l; return 4;
	case 0x7F: reg->a = reg->a; return 4;

	case 0x70: bus_write(bus, reg->hl, reg->b); return 8;
	case 0x71: bus_write(bus, reg->hl, reg->c); return 8;
	case 0x72: bus_write(bus, reg->hl, reg->d); return 8;
	case 0x73: bus_write(bus, reg->hl, reg->e); return 8;
	case 0x74: bus_write(bus, reg->hl, reg->h); return 8;
	case 0x75: bus_write(bus, reg->hl, reg->l); return 8;
	case 0x77: bus_write(bus, reg->hl, reg->a); return 8;

	// XOR r
	case 0xA8: {
		uint8_t r = reg->a ^ reg->b;
		flag_con(reg, FLAG_Z, r == 0);
		reg->f &= 0b10000000;
		return 4;
	}
	case 0xA9: {
		uint8_t r = reg->a ^ reg->c;
		flag_con(reg, FLAG_Z, r == 0);
		reg->f &= 0b10000000;
		return 4;
	}
	case 0xAA: {
		uint8_t r = reg->a ^ reg->d;
		flag_con(reg, FLAG_Z, r == 0);
		reg->f &= 0b10000000;
		return 4;
	}
	case 0xAB: {
		uint8_t r = reg->a ^ reg->e;
		flag_con(reg, FLAG_Z, r == 0);
		reg->f &= 0b10000000;
		return 4;
	}
	case 0xAC: {
		uint8_t r = reg->a ^ reg->h;
		flag_con(reg, FLAG_Z, r == 0);
		reg->f &= 0b10000000;
		return 4;
	}
	case 0xAD: {
		uint8_t r = reg->a ^ reg->l;
		flag_con(reg, FLAG_Z, r == 0);
		reg->f &= 0b10000000;
		return 4;
	}
	case 0xAF: {
		uint8_t r = reg->a ^ reg->a;
		flag_con(reg, FLAG_Z, r == 0);
		reg->f &= 0b10000000;
		return 4;
	}

	// PUSH rr
	case 0xC5: {
		bus_write(bus, --reg->sp, msb_make(reg->bc));
		bus_write(bus, --reg->sp, lsb_make(reg->bc));
		return 16;
	}
	case 0xD5: {
		bus_write(bus, --reg->sp, msb_make(reg->de));
		bus_write(bus, --reg->sp, lsb_make(reg->de));
		return 16;
	}
	case 0xE5: {
		bus_write(bus, --reg->sp, msb_make(reg->hl));
		bus_write(bus, --reg->sp, lsb_make(reg->hl));
		return 16;
	}
	case 0xF5: {
		bus_write(bus, --reg->sp, msb_make(reg->af));
		bus_write(bus, --reg->sp, lsb_make(reg->af));
		return 16;
	}

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
		return cb_execute(bus, reg, opcode);
	}

	// LDH (C), A
	case 0xE2: bus_write(bus, u16(reg->c, 0xFF), reg->a); return 8;

	// CP n
	case 0xFE: {
		uint8_t n = bus_read(bus, reg->pc++);
		uint8_t r = reg->a - n;
		bool c = reg->a < n;
		bool hc = (reg->a & 0x0F) < (n & 0x0F);
		flag_con(reg, FLAG_Z, !r);
		flag_set(reg, FLAG_N);
		flag_con(reg, FLAG_H, hc);
		flag_con(reg, FLAG_C, c);
		return 12;
	}

	default:
		die("Instruction 0x%02X not implemented.\n", opcode);
		return 0;
	}
}

