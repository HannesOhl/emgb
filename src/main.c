#include "../inc/cpu.h"
#include "../inc/bus.h"
#include "../inc/util.h"

static char* pname;
static size_t n_fetches = 0;

int main(int argc, char** argv) {

	pname = argv[0];
	if (argc < 2) die("Usage: %s rom.gb\n", pname);

	FILE* rom_b = fopen("dmg_boot.bin", "rb");
	if (!rom_b) die("Error opening Boot ROM.\n");

	FILE* rom = fopen(argv[1], "rb");
	if (!rom) die("Error opening ROM.\n");

	Bus bus = {0};
	bus_init(&bus, rom);
	fclose(rom);

	Cpu cpu = {0};
	cpu_init(&cpu);

	while (true) {
		printf("\nfetches = %zu\n", n_fetches);
		uint32_t cycles = cpu_step(&cpu, &bus);
		n_fetches++;
		cpu.cycles += cycles / 4;
		stack_print(&cpu, &bus);
		cpu_state_print(&cpu);
	}

	free(bus.c_rom);
	return 0;
}

