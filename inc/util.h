#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static inline uint16_t u16(uint8_t lsb, uint8_t msb) {
	return lsb | (msb << 8);
}

static inline uint8_t lsb_make(uint16_t value) {
	return (uint8_t) (value & 0x00FF);
}

static inline uint8_t msb_make(uint16_t value) {
	return (uint8_t) (value >> 8);
}

static inline void die(const char* fmt, ...) {
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

#endif

