#ifndef CPU_H
#define CPU_H

#include "./bus.h"

#include <stdint.h>

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

typedef enum {
	FLAG_C = (1 << 4),
	FLAG_H = (1 << 5),
	FLAG_N = (1 << 6),
	FLAG_Z = (1 << 7)
} Flag;

typedef struct {
	Registers reg;

	bool halt;
	bool stop;

	uint64_t cycles;
} Cpu;

void flag_set(Registers* reg, Flag f);
void flag_clear(Registers* reg, Flag f);
void flag_cond(Registers* reg, Flag f, bool cond);

void stack_print(Cpu* cpu, Bus* bus);
void cpu_state_print(Cpu* cpu);

uint32_t cpu_step(Cpu* cpu, Bus* mem);
#endif

