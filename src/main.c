#include "../inc/cpu.h"
#include "../inc/bus.h"
#include "../inc/util.h"

static char* pname;

int main(int argc, char** argv) {

	pname = argv[0];
	if (argc < 2) {
		die("Usage: %s rom.gb\n", pname);
	}

	FILE* rom = fopen(argv[1], "rb");
	if (!rom) {
		die("Error opening ROM.\n");
	}

	Bus bus = {0};
	bus_init(&bus, rom);

	Cpu cpu = {0};
	cpu_init(&cpu);

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

