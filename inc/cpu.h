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

typedef struct {

	Registers reg;

	bool ime;
	bool ime_scheduled;
	bool halted;
	bool halt_bug;
	bool stopped;
} Cpu;

bool cpu_init(Cpu* cpu, Bus* bus);
uint32_t cpu_step(Cpu* cpu, Bus* bus);

#endif

