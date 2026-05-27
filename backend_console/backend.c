#include "../inc/backend.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include <unistd.h>
#include <poll.h>

#include <termios.h>

static const uint32_t SCREEN_WIDTH  = 160;
static const uint32_t SCREEN_HEIGHT = 144;

typedef struct EXTBackendContext {
	uint32_t* buffer;
	struct termios old_termios;
	bool have_pending_release;
	InputKey pending_key;
	uint32_t release_delay;
} EXTBackendContext;

EXTBackendContext* EXT_backend_init(uint32_t* buffer) {

	EXTBackendContext* ctx = calloc(1, sizeof *ctx);
	if (!ctx) return NULL;

	ctx->buffer = buffer;

	tcgetattr(STDIN_FILENO, &ctx->old_termios);
	struct termios raw = ctx->old_termios;
	cfmakeraw(&raw);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);

	ctx->release_delay = 8000;

	// clear screen, home cursor
	printf("\x1b[2J\x1b[H");

	// hide cursor
	printf("\x1b[?25l");

	return ctx;
}

void EXT_backend_destroy(EXTBackendContext* ctx) {

	if (!ctx) return;

	tcsetattr(STDIN_FILENO, TCSANOW, &ctx->old_termios);
	// reset colors, show cursor
	printf("\x1b[0m\x1b[?25h");
	fflush(stdout);

	free(ctx);
}

static int32_t ansi_gray(uint32_t rgb) {

	switch (rgb) {
	case 0xFFFFFF: return 15;
	case 0xAAAAAA: return 250;
	case 0x555555: return 242;
	case 0x000000: return 232;
	default: break;
	}
	return 0xFFAA55;
}

void EXT_backend_present(EXTBackendContext* ctx) {

	printf("\x1b[H");

	for (size_t y = 0; y < (size_t) SCREEN_HEIGHT; y += 2) {
		for (size_t x = 0; x < (size_t) SCREEN_WIDTH; x++) {

			size_t idx_top   = x + y * (size_t) SCREEN_WIDTH;
			uint32_t top_rgb = ctx->buffer[idx_top];

			size_t y_bot     = y + 1 < (size_t) SCREEN_HEIGHT ? y + 1 : y;
			size_t idx_bot   = x + y_bot * (size_t) SCREEN_WIDTH;
			uint32_t bot_rgb = ctx->buffer[idx_bot];

			int32_t shade_top = ansi_gray(top_rgb);
			int32_t shade_bot = ansi_gray(bot_rgb);

			printf("\x1b[38;5;%dm\x1b[48;5;%dm▀", shade_top, shade_bot);
		}
		printf("\x1b[0m\r\n");
	}
	fflush(stdout);
}

bool EXT_backend_input(EXTBackendContext* ctx, InputEvent* event) {

	if (ctx->have_pending_release) {
		if (ctx->release_delay == 0) {
			event->key = ctx->pending_key;
			event->pressed = false;
			ctx->have_pending_release = false;
			return true;
		} else {
			ctx->release_delay -= 1;
			return false;
		}
	}

	struct pollfd pfd = {
		.fd = STDIN_FILENO,
		.events = POLLIN,
		.revents = 0
	};

	if (poll(&pfd, 1, 0) <= 0) {
		return false;
	}

	uint8_t c;
	if (read(STDIN_FILENO, &c, 1) != 1) {
		return false;
	}

	switch (c) {
	case 'd': event->key = INPUT_RIGHT;  break;
	case 'a': event->key = INPUT_LEFT;   break;
	case 'w': event->key = INPUT_UP;     break;
	case 's': event->key = INPUT_DOWN;   break;
	case 'k': event->key = INPUT_A;      break;
	case 'j': event->key = INPUT_B;      break;
	case 'm': event->key = INPUT_START;  break;
	case 'c': event->key = INPUT_SELECT; break;
	case 'q': event->key = INPUT_QUIT;   break;
	default: return false;
	}

	event->pressed = true;

	ctx->pending_key = event->key;
	ctx->have_pending_release = true;
	ctx->release_delay = 8000;

	return true;
}

