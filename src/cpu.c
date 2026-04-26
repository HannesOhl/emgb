// TODO: maybe use _Generic to treat opcodes for (HL) different!

#include "../inc/cpu.h"

#include "../inc/util.h"

static void flag_set(Registers* reg, Flag f) {
	reg->f |= (uint8_t) f;
	reg->f &= 0xF0;
}
static void flag_clr(Registers* reg, Flag f) {
	reg->f &= (uint8_t) ~f;
	reg->f &= 0xF0;
}
static void flag_con(Registers* reg, Flag f, bool cond) {
	reg->f = cond ? (reg->f | (uint8_t) f) : (reg->f & (uint8_t) ~f);
	reg->f &= 0xF0;
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

	cpu->ime      	   = false;
	cpu->ime_scheduled = false;
	cpu->halted 	   = false;
	cpu->stopped 	   = false;
	cpu->halt_bug	   = false;
	cpu->reg.pc  	   = 0x0000;
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
	die("rlc.\n"); (void) reg;
	(void) r_val; return 0; }
uint8_t rrc(Registers* reg, uint8_t r_val) {
	die("rrc.\n"); (void) reg;
	(void) r_val; return 0; }
uint8_t rl (Registers* reg, uint8_t r_val) {
	uint8_t c_in   = flag_check(reg, FLAG_C);
	uint8_t c_out  = r_val >> 7;
	uint8_t result = (r_val << 1) | c_in;
	flag_con(reg, FLAG_C, c_out);
	flag_con(reg, FLAG_Z, result == 0);
	flag_clr(reg, FLAG_N);
	flag_clr(reg, FLAG_H);
	return result;
}
uint8_t rr (Registers* reg, uint8_t r_val) {
	die("rr .\n"); (void) reg;
	(void) r_val; return 0; }

uint8_t sla(Registers* reg, uint8_t r_val) {
	flag_con(reg, FLAG_C, r_val >> 7);
	uint8_t res = r_val << 1;
	flag_con(reg, FLAG_Z, res == 0 );
	flag_clr(reg, FLAG_N);
	flag_clr(reg, FLAG_H);
	return res;
}

uint8_t sra(Registers* reg, uint8_t r_val) {
	die("sra.\n"); (void) reg;
	(void) r_val; return 0; }
uint8_t swp(Registers* reg, uint8_t r_val) {

	uint8_t res = (r_val >> 4) | (r_val << 4);
	flag_con(reg, FLAG_Z, res == 0);
	flag_clr(reg, FLAG_N);
	flag_clr(reg, FLAG_H);
	flag_clr(reg, FLAG_C);
	return res;
}

uint8_t srl(Registers* reg, uint8_t r_val) {
	die("srl.\n"); (void) reg;
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
		cycles = (r != 6) ? 8 : 12;
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

static uint8_t add_r(Registers* reg, uint8_t r_val) {
	uint8_t res = reg->a + r_val;
	flag_con(reg, FLAG_Z, res == 0);
	flag_clr(reg, FLAG_N);
	flag_con(reg, FLAG_H, ((reg->a & 0x0F) + (r_val & 0x0F)) > 0x0F);
	flag_con(reg, FLAG_C, res < reg->a);
	return res;
}

uint8_t adc_r(Registers* reg, uint8_t r_val) {
	uint16_t c = flag_check(reg, FLAG_C) ? 1 : 0;
	uint16_t res = (uint16_t) ((uint16_t) reg->a + (uint16_t) r_val + c);
	flag_con(reg, FLAG_Z, ((res & 0xFF) == 0));
	flag_clr(reg, FLAG_N);
	flag_con(reg, FLAG_H, ((reg->a & 0x0F) + (r_val & 0x0F) + c > 0x0F));
	flag_con(reg, FLAG_C, res > 0xFF);
	return (uint8_t) res;
}

static uint8_t and_r(Registers* reg, uint8_t r_val) {
	uint8_t res = reg->a & r_val;
	flag_con(reg, FLAG_Z, res == 0);
	flag_clr(reg, FLAG_N);
	flag_set(reg, FLAG_H);
	flag_clr(reg, FLAG_C);
	return res;
}

static uint8_t xor_r(Registers* reg, uint8_t r_val) {
	uint8_t res = reg->a ^ r_val;
	flag_con(reg, FLAG_Z, res == 0);
	reg->f &= 0b10000000;
	return res;
}

static uint8_t sbc_r(Registers* reg, uint8_t r_val) {
	uint16_t car = (uint16_t) flag_check(reg, FLAG_C);
	uint16_t sum = (uint16_t) r_val + (uint16_t) car;
	uint16_t res = (uint16_t) reg->a - sum;

	flag_con(reg, FLAG_Z, (uint8_t) res == 0);
	flag_set(reg, FLAG_N);
	flag_con(reg, FLAG_H, (reg->a & 0x0F) < ((r_val & 0x0F) + (uint8_t) car));
	flag_con(reg, FLAG_C, reg->a < sum);

	return (uint8_t) res;
}

static void cp_r(Registers* reg, uint8_t r_val) {
	uint8_t res = reg->a - r_val;
	flag_con(reg, FLAG_Z, res == 0);
	flag_set(reg, FLAG_N);
	flag_con(reg, FLAG_H, (reg->a & 0x0F) < (r_val & 0x0F));
	flag_con(reg, FLAG_C, reg->a < r_val);
}

static uint8_t inc_r(Registers* reg, uint8_t r_val) {

	uint8_t result = r_val + 1;
	flag_con(reg, FLAG_H, (r_val & 0x0F) == 0x0F);
	flag_con(reg, FLAG_Z, result == 0);
	flag_clr(reg, FLAG_N);

	return result;
}

static uint8_t dec_r(Registers* reg, uint8_t r) {

	uint8_t result = r - 1;
	flag_con(reg, FLAG_H, (r & 0x0F) == 0x00);
	flag_con(reg, FLAG_Z, result == 0);
	flag_set(reg, FLAG_N);

	return result;
}

static uint16_t dec_rr(uint16_t r) {
	return r - 1;
}

static uint8_t or_r(Registers* reg, uint8_t r_val) {
	uint8_t res = reg->a | r_val;
	flag_con(reg, FLAG_Z, res == 0);
	flag_clr(reg, FLAG_N);
	flag_clr(reg, FLAG_H);
	flag_clr(reg, FLAG_C);

	return res;
}

static uint8_t sub_r(Registers* reg, uint8_t r_val) {
	uint8_t a = reg->a;
	uint8_t res = a - r_val;
	flag_con(reg, FLAG_H, (a & 0x0F) < (r_val & 0x0F));
	flag_con(reg, FLAG_C, a < r_val);
	flag_con(reg, FLAG_Z, res == 0);
	flag_set(reg, FLAG_N);
	return res;
}

static void interrupt_handle(Cpu* cpu, Bus* bus, bool* occured) {

	Registers* reg = &cpu->reg;

	uint8_t hw_ie = bus_read(bus, HW_REG_IE);
	uint8_t hw_if = bus_read(bus, HW_REG_IF);
	uint8_t requested = hw_ie & hw_if & 0x1F;
	if (!requested) {
		*occured = false;
		return;
	}

	cpu->ime = false;
	cpu->ime_scheduled = false;

	uint8_t v = 0;
	for (size_t i = 0; i < 5; i++) {
		if (requested & (1 << i)) {
			uint8_t r = bus_read(bus, HW_REG_IF);
			bus_write(bus, HW_REG_IF, r &= (uint8_t) ~(1 << i));
			v = (uint8_t) ((uint8_t) 0x40 + (uint8_t) i * (uint8_t) 0x08);
			break;
		}
	}

	reg->sp -= 2;
	// Write hi first (hardware behaviour for push: SP-1 = hi, SP-2 = lo)
	bus_write(bus, reg->sp + 1, (uint8_t) (reg->pc >> 8));
	bus_write(bus, reg->sp    , (uint8_t) (reg->pc & 0xFF));
	reg->pc = v;
	*occured = true;
}


uint32_t cpu_step(Cpu* cpu, Bus* bus) {

	Registers* reg = &cpu->reg;

	bool ei_delay = cpu->ime_scheduled;

	// handle STOP state
	if (cpu->stopped) {
		uint8_t r = bus_read(bus, HW_REG_IF);
		bool interrupt_flag = r & 0b00010000;
		if (interrupt_flag) {
			cpu->stopped = false;
			bus_write(bus, HW_REG_IF, r &= (uint8_t) ~0b00010000);
		} else {
			return 4;
		}
	}

	// handle HALT state
	if (cpu->halted) {
		uint8_t hw_ie = bus_read(bus, HW_REG_IE);
		uint8_t hw_if = bus_read(bus, HW_REG_IF);
		uint8_t requested = hw_ie & hw_if & 0x1F;
		if (!requested) {
			return 4;
		}

		if (ei_delay && cpu->ime_scheduled) {
			cpu->ime = true;
			cpu->ime_scheduled = false;
		}
		cpu->halted = false;
		if (cpu->ime) {
			bool occured = false;
			interrupt_handle(cpu, bus, &occured);
			return 20;
		} else {
			// execute next instruction
		}
	}

	// handle interrupts
	if (cpu->ime) {
		bool occured = false;
		interrupt_handle(cpu, bus, &occured);
		if (occured) return 20;
	}

	uint8_t opcode = 0;
	if (cpu->halt_bug) {
		opcode = bus_read(bus, reg->pc);
		cpu->halt_bug = false;
	} else {
		opcode = bus_read(bus, reg->pc++);
	}
	printf("executing opcode %02X\n", opcode);

	if (ei_delay) {
		cpu->ime = true;
		cpu->ime_scheduled = false;
	}

	switch (opcode) {

	// NOP 1/1
	case 0x00: return 4;

	// (post boot rom) ADD HL, rr
	case 0x09: {
		uint16_t res = reg->hl + reg->bc;
		flag_clr(reg, FLAG_N);
		flag_con(reg, FLAG_H, ((reg->hl & 0x0FFF) + (reg->bc & 0x0FFF)) > 0x0FFF);
		flag_con(reg, FLAG_C, res < reg->hl);
		reg->hl = res;
		return 8;
	}
	case 0x19: {
		uint16_t res = reg->hl + reg->de;
		flag_clr(reg, FLAG_N);
		flag_con(reg, FLAG_H, ((reg->hl & 0x0FFF) + (reg->de & 0x0FFF)) > 0x0FFF);
		flag_con(reg, FLAG_C, res < reg->hl);
		reg->hl = res;
		return 8;
	}
	case 0x29: {
		uint16_t res = reg->hl + reg->hl;
		flag_clr(reg, FLAG_N);
		flag_con(reg, FLAG_H, ((reg->hl & 0x0FFF) + (reg->hl & 0x0FFF)) > 0x0FFF);
		flag_con(reg, FLAG_C, res < reg->hl);
		reg->hl = res;
		return 8;
	}
	case 0x39: {
		uint16_t res = reg->hl + reg->sp;
		flag_clr(reg, FLAG_N);
		flag_con(reg, FLAG_H, ((reg->hl & 0x0FFF) + (reg->sp & 0x0FFF)) > 0x0FFF);
		flag_con(reg, FLAG_C, res < reg->hl);
		reg->hl = res;
		return 8;
	}

	// (post boot rom) STOP
	// TODO: handle this correctly, (cgb mode???)
	case 0x10: {
		uint8_t consume = bus_read(bus, reg->pc++);
		(void) consume;
		cpu->stopped = true;
		return 4;
	}

	// (post boot rom) LD (DE), A
	case 0x12: bus_write(bus, reg->de, reg->a); return 8;

	// INC rr
	case 0x03: reg->bc += 1; return 8;
	case 0x13: reg->de += 1; return 8;
	case 0x23: reg->hl += 1; return 8;
	case 0x33: reg->sp += 1; return 8;


	// (post boot rom) INC (HL)
	case 0x34: {
		uint8_t data = bus_read(bus, reg->hl);
		uint8_t res  = data + 1;
		bus_write(bus, reg->hl, res);
		flag_con(reg, FLAG_Z, res == 0);
		flag_clr(reg, FLAG_N);
		flag_con(reg, FLAG_Z, ((data & 0x0F) + 1) > 0x0F);
		return 12;
	}
	// (post boot rom) DEC (HL)
	case 0x35: {
		uint8_t data = bus_read(bus, reg->hl);
		uint8_t res  = data - 1;
		bus_write(bus, reg->hl, res);
		flag_con(reg, FLAG_Z, res == 0);
		flag_set(reg, FLAG_N);
		flag_con(reg, FLAG_Z, (data & 0x0F) == 0);
		return 12;
	}


	// INC r
	case 0x04: reg->b = inc_r(reg, reg->b); return 4;
	case 0x14: reg->d = inc_r(reg, reg->d); return 4;
	case 0x24: reg->h = inc_r(reg, reg->h); return 4;
	case 0x0C: reg->c = inc_r(reg, reg->c); return 4;
	case 0x1C: reg->e = inc_r(reg, reg->e); return 4;
	case 0x2C: reg->l = inc_r(reg, reg->l); return 4;
	case 0x3C: reg->a = inc_r(reg, reg->a); return 4;

	// DEC r
	case 0x05: reg->b = dec_r(reg, reg->b); return 4;
	case 0x15: reg->d = dec_r(reg, reg->d); return 4;
	case 0x25: reg->h = dec_r(reg, reg->h); return 4;
	case 0x0D: reg->c = dec_r(reg, reg->c); return 4;
	case 0x1D: reg->e = dec_r(reg, reg->e); return 4;
	case 0x2D: reg->l = dec_r(reg, reg->l); return 4;
	case 0x3D: reg->a = dec_r(reg, reg->a); return 4;

	// (post boot rom) DEC rr
	case 0x0B: reg->bc = dec_rr(reg->bc); return 8;
	case 0x1B: reg->de = dec_rr(reg->de); return 8;
	case 0x2B: reg->hl = dec_rr(reg->hl); return 8;
	case 0x3B: reg->sp = dec_rr(reg->sp); return 8;

	// (post boot rom) RLCA
	case 0x07: {
		uint8_t c_in = reg->a >> 7;
		reg->a = (reg->a << 1) | c_in;
                flag_con(reg, FLAG_C, c_in);
		flag_clr(reg, FLAG_Z);
		flag_clr(reg, FLAG_N);
		flag_clr(reg, FLAG_H);
		return 4;
	}

	// RLA
	case 0x17: {
		uint8_t c_in   = flag_check(reg, FLAG_C);
		uint8_t c_out  = reg->a >> 7;
		reg->a = (reg->a << 1) | c_in;
		flag_con(reg, FLAG_C, c_out);
		flag_clr(reg, FLAG_Z);
		flag_clr(reg, FLAG_N);
		flag_clr(reg, FLAG_H);
		return 4;
	}

	// JR e
	case 0x18 : {
		int8_t e = (int8_t) bus_read(bus, reg->pc++);
		reg->pc = (uint16_t) (reg->pc + (int32_t) e);
		return 12;
	}

	// JR cc, e 4/
	case 0x20: {
		int8_t e = (int8_t) bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_Z)) {
			reg->pc = (uint16_t) (reg->pc + (int32_t) e);
			return 12;
		}
		return 8;
	}
	case 0x28: {
		int8_t e = (int8_t) bus_read(bus, reg->pc++);
		if (flag_check(reg, FLAG_Z)) {
			reg->pc = (uint16_t) (reg->pc + (int32_t) e);
			return 12;
		}
		return 8;
	}
	case 0x30: {
		int8_t e = (int8_t) bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_C)) {
			reg->pc = (uint16_t) (reg->pc + (int32_t) e);
			return 12;
		}
		return 8;
	}
	case 0x38: {
		int8_t e = (int8_t) bus_read(bus, reg->pc++);
		if (flag_check(reg, FLAG_C)) {
			reg->pc = (uint16_t) (reg->pc + (int32_t) e);
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

	// (post boot rom) CPL
	case 0x2F: {
		reg->a = ~reg->a;
		flag_set(reg, FLAG_N);
		flag_set(reg, FLAG_H);
		return 4;
	}

	// LD r, r' 49/49
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

	// (post boot rom) LD r, (HL)
	case 0x46: reg->b = bus_read(bus, reg->hl); return 8;
	case 0x4E: reg->c = bus_read(bus, reg->hl); return 8;
	case 0x56: reg->d = bus_read(bus, reg->hl); return 8;
	case 0x5E: reg->e = bus_read(bus, reg->hl); return 8;
	case 0x66: reg->h = bus_read(bus, reg->hl); return 8;
	case 0x6E: reg->l = bus_read(bus, reg->hl); return 8;
	case 0x7E: reg->a = bus_read(bus, reg->hl); return 8;

	// LD (HL), r
	case 0x70: bus_write(bus, reg->hl, reg->b); return 8;
	case 0x71: bus_write(bus, reg->hl, reg->c); return 8;
	case 0x72: bus_write(bus, reg->hl, reg->d); return 8;
	case 0x73: bus_write(bus, reg->hl, reg->e); return 8;
	case 0x74: bus_write(bus, reg->hl, reg->h); return 8;
	case 0x75: bus_write(bus, reg->hl, reg->l); return 8;
	case 0x77: bus_write(bus, reg->hl, reg->a); return 8;

	// (post boot rom) HALT
	case 0x76: {
		uint8_t pending = bus_read(bus, 0xFFFF) & bus_read(bus, 0xFF0F) & 0x1F;
		if (cpu->ime || pending == 0) {
			cpu->halted = true;
		} else {
			cpu->halt_bug = true;
		}
		return 4;
	}

	// (post boot rom) ADD r
	case 0x80: reg->a = add_r(reg, reg->b); return 4;
	case 0x81: reg->a = add_r(reg, reg->c); return 4;
	case 0x82: reg->a = add_r(reg, reg->d); return 4;
	case 0x83: reg->a = add_r(reg, reg->e); return 4;
	case 0x84: reg->a = add_r(reg, reg->h); return 4;
	case 0x85: reg->a = add_r(reg, reg->l); return 4;
	case 0x87: reg->a = add_r(reg, reg->a); return 4;

	// (post boot rom) ADC r
	case 0x88: reg->a = adc_r(reg, reg->b); return 4;
	case 0x89: reg->a = adc_r(reg, reg->c); return 4;
	case 0x8A: reg->a = adc_r(reg, reg->d); return 4;
	case 0x8B: reg->a = adc_r(reg, reg->e); return 4;
	case 0x8C: reg->a = adc_r(reg, reg->h); return 4;
	case 0x8D: reg->a = adc_r(reg, reg->l); return 4;
	case 0x8F: reg->a = adc_r(reg, reg->a); return 4;

	// ADD (HL)
	case 0x86: {
		uint8_t data = bus_read(bus, reg->hl);
		uint8_t res = reg->a + data;
		flag_con(reg, FLAG_Z, res == 0);
		flag_clr(reg, FLAG_N);
		flag_con(reg, FLAG_H, ((reg->a & 0x0F) + (data & 0x0F)) > 0x0F);
		flag_con(reg, FLAG_C, res < reg->a);
		reg->a = res;
		return 8;
	}

	// SUB r
	case 0x90: reg->a = sub_r(reg, reg->b); return 4;
	case 0x91: reg->a = sub_r(reg, reg->c); return 4;
	case 0x92: reg->a = sub_r(reg, reg->d); return 4;
	case 0x93: reg->a = sub_r(reg, reg->e); return 4;
	case 0x94: reg->a = sub_r(reg, reg->h); return 4;
	case 0x95: reg->a = sub_r(reg, reg->l); return 4;
	case 0x97: reg->a = sub_r(reg, reg->a); return 4;

	// (post boot rom) SBC r
	case 0x98: reg->a = sbc_r(reg, reg->b); return 4;
	case 0x99: reg->a = sbc_r(reg, reg->c); return 4;
	case 0x9A: reg->a = sbc_r(reg, reg->d); return 4;
	case 0x9B: reg->a = sbc_r(reg, reg->e); return 4;
	case 0x9C: reg->a = sbc_r(reg, reg->h); return 4;
	case 0x9D: reg->a = sbc_r(reg, reg->l); return 4;
	case 0x9F: reg->a = sbc_r(reg, reg->a); return 4;

	// (post boot rom) AND r
	case 0xA0: reg->a = and_r(reg, reg->b); return 4;
	case 0xA1: reg->a = and_r(reg, reg->c); return 4;
	case 0xA2: reg->a = and_r(reg, reg->d); return 4;
	case 0xA3: reg->a = and_r(reg, reg->e); return 4;
	case 0xA4: reg->a = and_r(reg, reg->h); return 4;
	case 0xA5: reg->a = and_r(reg, reg->l); return 4;
	case 0xA7: reg->a = and_r(reg, reg->a); return 4;

	// XOR r
	case 0xA8: reg->a = xor_r(reg, reg->b); return 4;
	case 0xA9: reg->a = xor_r(reg, reg->c); return 4;
	case 0xAA: reg->a = xor_r(reg, reg->d); return 4;
	case 0xAB: reg->a = xor_r(reg, reg->e); return 4;
	case 0xAC: reg->a = xor_r(reg, reg->h); return 4;
	case 0xAD: reg->a = xor_r(reg, reg->l); return 4;
	case 0xAF: reg->a = xor_r(reg, reg->a); return 4;

	// (post boot rom) OR r
	case 0xB0: reg->a = or_r(reg, reg->b); return 4;
	case 0xB1: reg->a = or_r(reg, reg->c); return 4;
	case 0xB2: reg->a = or_r(reg, reg->d); return 4;
	case 0xB3: reg->a = or_r(reg, reg->e); return 4;
	case 0xB4: reg->a = or_r(reg, reg->h); return 4;
	case 0xB5: reg->a = or_r(reg, reg->l); return 4;
	case 0xB7: reg->a = or_r(reg, reg->a); return 4;

	// (post boot rom) CP r
	case 0xB8: cp_r(reg, reg->b); return 4;
	case 0xB9: cp_r(reg, reg->c); return 4;
	case 0xBA: cp_r(reg, reg->d); return 4;
	case 0xBB: cp_r(reg, reg->e); return 4;
	case 0xBC: cp_r(reg, reg->h); return 4;
	case 0xBD: cp_r(reg, reg->l); return 4;
	case 0xBF: cp_r(reg, reg->a); return 4;

	// CP (HL)
	case 0xBE: {
		uint8_t r = bus_read(bus, reg->hl);
		flag_con(reg, FLAG_Z, reg->a == r);
		flag_set(reg, FLAG_N);
		flag_con(reg, FLAG_H, (reg->a & 0xF) < (r & 0xF));
		flag_con(reg, FLAG_C, reg->a < r);
		return 8;
	}

	// POP rr
	case 0xC1: {
		uint8_t lsb = bus_read(bus, reg->sp++);
		uint8_t msb = bus_read(bus, reg->sp++);
		reg->bc = u16(lsb, msb);
		return 12;
	}
	case 0xD1: {
		uint8_t lsb = bus_read(bus, reg->sp++);
		uint8_t msb = bus_read(bus, reg->sp++);
		reg->de = u16(lsb, msb);
		return 12;
	}
	case 0xE1: {
		uint8_t lsb = bus_read(bus, reg->sp++);
		uint8_t msb = bus_read(bus, reg->sp++);
		reg->hl = u16(lsb, msb);
		return 12;
	}
	case 0xF1: {
		uint8_t lsb = bus_read(bus, reg->sp++);
		uint8_t msb = bus_read(bus, reg->sp++);
		reg->af = u16(lsb, msb);
		reg->f &= 0xF0;
		return 12;
	}

	// (post boot rom) JP cc, nn
	case 0xC2: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_Z)) {
			reg->pc = u16(lsb, msb);
			return 16;
		} else {
			return 12;
		}
	}
	case 0xCA: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (flag_check(reg, FLAG_Z)) {
			reg->pc = u16(lsb, msb);
			return 16;
		} else {
			return 12;
		}
	}
	case 0xD2: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_C)) {
			reg->pc = u16(lsb, msb);
			return 16;
		} else {
			return 12;
		}
	}
	case 0xDA: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (flag_check(reg, FLAG_C)) {
			reg->pc = u16(lsb, msb);
			return 16;
		} else {
			return 12;
		}
	}

	// (post boot rom) CALL cc, nn
	case 0xC4: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_Z)) {
			bus_write(bus, --reg->sp, msb_make(reg->pc));
			bus_write(bus, --reg->sp, lsb_make(reg->pc));
			reg->pc = u16(lsb, msb);
			return 24;
		}
		return 12;
	}
	case 0xCC: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (flag_check(reg, FLAG_Z)) {
			bus_write(bus, --reg->sp, msb_make(reg->pc));
			bus_write(bus, --reg->sp, lsb_make(reg->pc));
			reg->pc = u16(lsb, msb);
			return 24;
		}
		return 12;
	}
	case 0xD4: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (!flag_check(reg, FLAG_C)) {
			bus_write(bus, --reg->sp, msb_make(reg->pc));
			bus_write(bus, --reg->sp, lsb_make(reg->pc));
			reg->pc = u16(lsb, msb);
			return 24;
		}
		return 12;
	}
	case 0xDC: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		if (flag_check(reg, FLAG_C)) {
			bus_write(bus, --reg->sp, msb_make(reg->pc));
			bus_write(bus, --reg->sp, lsb_make(reg->pc));
			reg->pc = u16(lsb, msb);
			return 24;
		}
		return 12;
	}

	// (post boot rom) JP HL
	case 0xE9: reg->pc = reg->hl; return 4;

 	// (post boot rom) JP nn
	case 0xC3: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		reg->pc = u16(lsb, msb);
		return 16;
	} break;

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

	// (post boot rom) ADD n
	case 0xC6 : {
		uint8_t n = bus_read(bus, reg->pc++);
		uint16_t res = reg->a + n;
		flag_con(reg, FLAG_Z, (res & 0xFF) == 0);
		flag_clr(reg, FLAG_N);
		flag_con(reg, FLAG_H, ((reg->a & 0x0F) + (n & 0x0F)) > 0x0F);
		flag_con(reg, FLAG_C, res > 0xFF);
		reg->a = (uint8_t) res;
		return 8;
	}

	//(post boot rom) SUB n
	case 0xD6 : {
		uint8_t n = bus_read(bus, reg->pc++);
		uint8_t res = reg->a - n;
		flag_con(reg, FLAG_Z, res == 0);
		flag_set(reg, FLAG_N);
		flag_con(reg, FLAG_H, (reg->a & 0x0F) < (n & 0x0F));
		flag_con(reg, FLAG_C, reg->a < n);
		reg->a = res;
		return 8;
	}

	// (post boot rom) RST n
	case 0xC7: {
		uint8_t n = 0x00;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}
	case 0xCF: {
		uint8_t n = 0x08;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}
	case 0xD7: {
		uint8_t n = 0x10;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}
	case 0xDF: {
		uint8_t n = 0x18;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}
	case 0xE7: {
		uint8_t n = 0x20;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}
	case 0xEF: {
		uint8_t n = 0x28;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}
	case 0xF7: {
		uint8_t n = 0x30;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}
	case 0xFF: {
		uint8_t n = 0x38;
		bus_write(bus, --reg->sp, msb_make(reg->pc));
		bus_write(bus, --reg->sp, lsb_make(reg->pc));
		reg->pc = u16(n, 0x00);
		return 16;
	}

	// (post boot rom) RET cc
	case 0xC0: {
		if (!flag_check(reg, FLAG_Z)) {
			uint8_t lsb = bus_read(bus, reg->sp++);
			uint8_t msb = bus_read(bus, reg->sp++);
			reg->pc = u16(lsb, msb);
			return 20;
		} else {
			return 8;
		}
	}
	case 0xC8: {
		if (flag_check(reg, FLAG_Z)) {
			uint8_t lsb = bus_read(bus, reg->sp++);
			uint8_t msb = bus_read(bus, reg->sp++);
			reg->pc = u16(lsb, msb);
			return 20;
		} else {
			return 8;
		}
	}
	case 0xD0: {
		if (!flag_check(reg, FLAG_C)) {
			uint8_t lsb = bus_read(bus, reg->sp++);
			uint8_t msb = bus_read(bus, reg->sp++);
			reg->pc = u16(lsb, msb);
			return 20;
		} else {
			return 8;
		}
	}
	case 0xD8: {
		if (flag_check(reg, FLAG_C)) {
			uint8_t lsb = bus_read(bus, reg->sp++);
			uint8_t msb = bus_read(bus, reg->sp++);
			reg->pc = u16(lsb, msb);
			return 20;
		} else {
			return 8;
		}
	}

	// RET
	case 0xC9: {
		uint8_t lsb = bus_read(bus, reg->sp++);
		uint8_t msb = bus_read(bus, reg->sp++);
		reg->pc = u16(lsb, msb);
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
	// (post boot rom) RETI
	case 0xD9 : {
		uint8_t lsb = bus_read(bus, reg->sp++);
		uint8_t msb = bus_read(bus, reg->sp++);
		reg->pc = u16(lsb, msb);
		cpu->ime = true;
		return 16;
	}

	// LD (nn), A
	case 0xEA: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		uint16_t nn = u16(lsb, msb);
		bus_write(bus, nn, reg->a);
		return 16;
	}

	// (post boot rom) AND n
	case 0xE6: {
		uint8_t n = bus_read(bus, reg->pc++);
		uint8_t res = reg->a & n;
		reg->a = res;
		flag_con(reg, FLAG_Z, res == 0);
		flag_clr(reg, FLAG_N);
		flag_set(reg, FLAG_H);
		flag_clr(reg, FLAG_C);
		return 8;
	}
	// (post boot rom) OR n
	case 0xF6: {
		uint8_t n = bus_read(bus, reg->pc++);
		uint8_t res = reg->a | n;
		reg->a = res;
		flag_con(reg, FLAG_Z, res == 0);
		flag_clr(reg, FLAG_N);
		flag_clr(reg, FLAG_H);
		flag_clr(reg, FLAG_C);
		return 8;
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

	case 0xCB: {
		opcode = bus_read(bus, reg->pc++);
		return cb_execute(bus, reg, opcode);
	}

	// LDH (C), A
	case 0xE2: bus_write(bus, u16(reg->c, 0xFF), reg->a); return 8;

	// (post boot rom) DI
	case 0xF3: cpu->ime = false; return 4;

	// (post boot rom) LD A, (nn)
	case 0xFA: {
		uint8_t lsb = bus_read(bus, reg->pc++);
		uint8_t msb = bus_read(bus, reg->pc++);
		reg->a = bus_read(bus, u16(lsb, msb));
		return 16;
	}
	// (post boot rom) EI
	case 0xFB: cpu->ime_scheduled = true; return 4;

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

