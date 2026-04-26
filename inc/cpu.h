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
	bool ime;
	bool ime_scheduled;
	bool halted;
	bool halt_bug;
	bool stopped;

	uint64_t cycles;
} Cpu;

void stack_print(Cpu* cpu, Bus* bus);
void cpu_state_print(Cpu* cpu);
void cpu_init(Cpu* cpu);
uint32_t cpu_step(Cpu* cpu, Bus* mem);
#endif

