#include "../inc/backend_sdl.h"
#include "../inc/cpu.h"
#include "../inc/bus.h"
#include "../inc/ppu.h"
#include "../inc/timer.h"
#include "../inc/util.h"

static char* pname;

static void joypad_handle(Bus* bus, SDL_KeyCode key, bool pressed) {

	switch (key) {

	case SDLK_RIGHT: bus_joypad_set(bus, JOY_A, pressed); break;
	default: break;

	}
}

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
	(void) buffer;

	Ppu ppu = {0};
	ppu_init(&ppu, &bus, buffer);

	Tmr tmr = {0};
	tmr_init(&tmr, &bus);

	bool running = true;
	while (running) {
		//if (!bus.b_enabled) die("turned of dmg rom.\n");
		while (SDL_PollEvent(&ctx->event) != 0) {
			switch (ctx->event.type) {
			case SDL_KEYDOWN: {
				if (ctx->event.key.keysym.sym == SDLK_ESCAPE) running = false;
				joypad_handle(&bus, ctx->event.key.keysym.sym, true);
			} break;
			case SDL_KEYUP: {
				joypad_handle(&bus, ctx->event.key.keysym.sym, false);
			} break;
			default: {} break;
			}
		}

		uint32_t cycles = cpu_step(&cpu, &bus);
		ppu_step(&ppu, ctx, cycles);
		tmr_step(&tmr, &bus, cycles);
		//stack_print(&cpu, &bus);
	}

	context_sdl_destroy(ctx);
	SDL_Quit();
	free(bus.c_rom);

	return 0;
}

