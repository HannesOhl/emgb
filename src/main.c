#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define MAX_ROM_SIZE 32000000

static inline void die(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	if (fmt) vfprintf(stderr, fmt, ap);
	if (!fmt) fprintf(stderr, "Unknown error.\n");
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	}

	exit(EXIT_FAILURE);
}

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
Registers reg;

int main(int argc, char** argv) {

	if (argc < 2) {
		die("Usage: \n");
	}

	uint8_t* car = calloc((size_t) MAX_ROM_SIZE, sizeof *car);
	if (!car) {
		die("Error allocating memory for ROM.\n");
	}

	FILE* rom = fopen(argv[1], "rb");
	if (!rom) {
		die("Error opening ROM.\n");
	}

	size_t ret = fread(car, 1, MAX_ROM_SIZE, rom);
	(void) ret;


	free(car);
	fclose(rom);
	return 0;
}

