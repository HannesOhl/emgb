#include "../inc/backend_sdl.h"
#include "../inc/cpu.h"
#include "../inc/bus.h"
#include "../inc/util.h"

static char* pname;
static size_t n_fetches = 0;

int main(int argc, char** argv) {

	pname = argv[0];
	if (argc < 2) die("Usage: %s rom.gb\n", pname);

	FILE* rom_b = fopen("./dmg_boot.bin", "rb");
	if (!rom_b) die("Error opening Boot ROM.\n");

	FILE* rom = fopen(argv[1], "rb");
	if (!rom) die("Error opening ROM.\n");

	Bus bus = {0};
	bus_init(&bus, rom_b, rom);
	fclose(rom);
	fclose(rom_b);

	Cpu cpu = {0};
	cpu_init(&cpu);

	SDLContext* ctx = calloc((size_t) 1, (size_t) sizeof *ctx);
	context_sdl_init(ctx);
	uint32_t* buffer = ctx->surface->pixels;

	while (true) {
		printf("\nfetches = %zu\n", n_fetches);
		cpu_state_print(&cpu);
		uint32_t cycles = cpu_step(&cpu, &bus);
		(void) cycles;
		n_fetches++;
		//stack_print(&cpu, &bus);
	}

	context_sdl_destroy(ctx);
	SDL_Quit();

	free(bus.c_rom);
	return 0;
}

