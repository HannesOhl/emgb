#include "../inc/backend.h"

#include "./SDL2/include/SDL.h"

#ifdef DEBUG
#include <sanitizer/lsan_interface.h>
#endif

static const uint32_t SCREEN_WIDTH  = 160;
static const uint32_t SCREEN_HEIGHT = 144;

struct EXTBackendContext {
	SDL_Window*  window;
	SDL_Surface* surface;
	SDL_Event    event;
	uint32_t*    buffer;
	uint32_t     bytes_per_pixel;
};

EXTBackendContext* EXT_backend_init(uint32_t* buffer) {

	// disable lsan (to suppress SDL memory leak errors)
	#ifdef DEBUG
	__lsan_disable();
	#endif

	EXTBackendContext* ctx = calloc((size_t) 1, sizeof *ctx);

	ctx->buffer = buffer;

	SDL_Init(SDL_INIT_VIDEO);
	ctx->window = SDL_CreateWindow("SoftRend", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						   (int32_t) SCREEN_WIDTH, (int32_t) SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (!ctx->window) fprintf(stderr, "Failed to create window. Error: %s\n", SDL_GetError());

	ctx->surface = SDL_GetWindowSurface(ctx->window);
	ctx->bytes_per_pixel = ctx->surface->format->BytesPerPixel;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	// re-enable lsan
	#ifdef DEBUG
	__lsan_enable();
	#endif

	return ctx;
}

void EXT_backend_destroy(EXTBackendContext* ctx) {

	SDL_DestroyWindow(ctx->window);
	free(ctx);
}

void EXT_backend_present(EXTBackendContext* ctx) {
	memcpy(ctx->surface->pixels, ctx->buffer, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
	SDL_UpdateWindowSurface(ctx->window);
}

bool EXT_backend_input(EXTBackendContext* ctx, InputEvent* event) {

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
			bool pressed = (e.type == SDL_KEYDOWN);

			switch (e.key.keysym.sym) {
			case SDLK_RIGHT:  event->key = INPUT_RIGHT;  break;
			case SDLK_LEFT:   event->key = INPUT_LEFT;   break;
			case SDLK_UP:     event->key = INPUT_UP;     break;
			case SDLK_DOWN:   event->key = INPUT_DOWN;   break;
			case SDLK_y:      event->key = INPUT_A;      break;
			case SDLK_x:      event->key = INPUT_B;      break;
			case SDLK_RETURN: event->key = INPUT_START;  break;
			case SDLK_RCTRL:  event->key = INPUT_SELECT; break;
			case SDLK_ESCAPE: event->key = INPUT_QUIT;   break;
			default: continue;
			}
			event->pressed = pressed;
			return true;
		}
	}
	return false;
}

