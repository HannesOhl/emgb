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

	// CALL nn
	case 0xCD: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		uint16_t nn = u16(lsb, msb);
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = nn;
		return 24;
	} break;

	// LDH A, (n)
	case 0xF0: {
		uint8_t n = bus_read(bus, reg->pc++);
		reg->a = bus_read(bus, u16(n, 0xFF));
		return 12;
	} break;

	// LDH (n), A
	case 0xE0: {
		uint8_t n = bus_read(bus, reg->pc++);
		bus_write(bus, u16(n, 0xFF), reg->a);
		return 12;
	} break;

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
		} break;
		default:
			die("Instruction 0xCB 0x%hhX not implemented.\n", opcode);
			return 0;
			break;
		}
	} break;

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
	} break;

	default:
		die("Instruction 0x%hhX not implemented.\n", opcode);
		return 0;
		break;
	}
}

