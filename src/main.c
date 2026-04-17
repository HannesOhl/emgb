#include "../inc/cpu.h"
#include "../inc/bus.h"
#include "../inc/util.h"

#define MAX_ROM_SIZE 8000000

static char* pname;

int main(int argc, char** argv) {

	pname = argv[0];
	if (argc < 2) {
		die("Usage: %s rom.gb\n", pname);
	}

	Bus bus = {0};
	bus.c_rom = calloc((size_t) MAX_ROM_SIZE, sizeof *bus.c_rom);
	if (!bus.c_rom) {
		die("Error allocating busory for ROM.\n");
	}

	FILE* rom = fopen(argv[1], "rb");
	if (!rom) {
		die("Error opening ROM.\n");
	}

	size_t ret = fread(bus.c_rom, 1, MAX_ROM_SIZE, rom);
	(void) ret;

	Cpu cpu = {0};
	cpu.reg.pc = 0x0150;
	cpu.reg.sp = 0xFFFE;

	while (true) {
		uint32_t cycles = cpu_step(&cpu, &bus);
		(void) cycles;
		stack_print(&cpu, &bus);
		cpu_state_print(&cpu);
	}

	free(bus.c_rom);
	fclose(rom);
	return 0;
}

