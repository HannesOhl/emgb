// TODO: add ctx to emu struct
#include "../inc/main.h"

// core
#include "../inc/bus.h"
#include "../inc/cpu.h"
#include "../inc/ppu.h"
#include "../inc/tmr.h"
#include "../inc/backend.h"

#include <stdio.h>
#include <errno.h>

// for getopt()
#include <unistd.h>

// for die()
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


void die(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	if (fmt) vfprintf(stderr, fmt, ap);
	if (!fmt) fprintf(stderr, "Unknown error.\n");
	va_end(ap);

	if (fmt && fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}

	exit(EXIT_FAILURE);
}

void help_print(char** argv) {

	die("Usage: %s ... \n", argv[0]);
}

bool emu_init(Emulator* emu, FILE* rom_boot, FILE* rom_game) {

	emu->bus = calloc((size_t) 1, sizeof *emu->bus);
	if (!emu->bus) {
		fprintf(stderr, "[ERROR]: Memory allocation for bus failed.\n");
		return false;
	}
	if (!bus_init(emu->bus, rom_boot, rom_game)) {
		return false;
	}

	emu->cpu = calloc((size_t) 1, sizeof *emu->cpu);
	if (!emu->cpu) {
		fprintf(stderr, "[ERROR]: Memory allocation for cpu failed.\n");
		return false;
	}
	if (!cpu_init(emu->cpu, emu->bus)) {
		return false;
	}

	emu->ppu = calloc((size_t) 1, sizeof *emu->ppu);
	if (!emu->ppu) {
		fprintf(stderr, "[ERROR]: Memory allocation for ppu failed.\n");
		return false;
	}
	if (!ppu_init(emu->ppu)) {
		return false;
	}

	emu->tmr = calloc((size_t) 1, sizeof *emu->tmr);
	if (!emu->tmr) {
		fprintf(stderr, "[ERROR]: Memory allocation for tmr failed.\n");
		return false;
	}
	tmr_init(emu->tmr, emu->bus);

	return true;
}

int main(int argc, char** argv) {

	int opt;
	while ((opt = getopt(argc, argv, "h")) != -1) {

		switch (opt) {
		case 'h':
			help_print(argv);
			break;
		default:
			break;
		}
	}

	FILE* rom_boot = NULL;
	FILE* rom_game = NULL;

	// decl. and init. this here: gcc -Wmaybe-uninitialised false pos.
	Emulator emu = {0};
	EXTBackendContext* ctx = NULL;

	// open rom passed as command line argument
	if (argc == 2) {
		rom_boot = fopen("./roms/dmg_boot.bin", "rb");
		if (!rom_boot) {
			printf("[INFO]: No boot-rom found. We skip it.\n");
		}

		rom_game = fopen(argv[1], "rb");
		if (!rom_game) {
			fprintf(stderr, "[ERROR]: Can't open %s: %s\n", argv[1], strerror(errno));
			goto cleanup;
		}

		if (!emu_init(&emu, rom_boot, rom_game)) goto cleanup;

		ctx = EXT_backend_init(emu.ppu->pixels);

		bool running = true;
		while (running) {

			// input handling
			InputEvent event = {0};
			if (EXT_backend_input(ctx, &event)) {
				input_handle(emu.bus, event.key, event.pressed, &running);
			}

			// stepping
			uint32_t cycles = cpu_step(emu.cpu, emu.bus);
			ppu_step(emu.ppu, emu.bus, ctx, cycles);
			tmr_step(emu.tmr, emu.bus, cycles);

		}
	} else {
		help_print(argv);
	}

cleanup:
	if (rom_boot) fclose(rom_boot);
	if (rom_game) fclose(rom_game);
	free(emu.tmr);
	free(emu.ppu->pixels);
	free(emu.ppu);
	free(emu.cpu);
	free(emu.bus);
	EXT_backend_destroy(ctx);
	return 0;
}

