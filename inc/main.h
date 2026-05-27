#ifndef _MAIN_H
#define _MAIN_H

#include "./bus.h"
#include "./cpu.h"
#include "./ppu.h"
#include "./tmr.h"

typedef struct {

	Bus* bus;
	Cpu* cpu;
	Ppu* ppu;
	Tmr* tmr;

} Emulator;

bool emu_init(Emulator* emu, FILE* rom_boot, FILE* rom_game);

#endif

