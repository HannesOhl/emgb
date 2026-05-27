#include "../inc/input.h"

void input_handle(Bus* bus, InputKey key, bool pressed, bool* running) {


	if (key == INPUT_QUIT) {
		*running = false;
		return;
	}

	uint8_t mask = 0;

	switch (key) {
	case INPUT_RIGHT:  mask = BUTTON_RIGHT;  break;
	case INPUT_LEFT:   mask = BUTTON_LEFT;   break;
	case INPUT_UP:     mask = BUTTON_UP;     break;
	case INPUT_DOWN:   mask = BUTTON_DOWN;   break;
	case INPUT_A:      mask = BUTTON_A;      break;
	case INPUT_B:      mask = BUTTON_B;      break;
	case INPUT_START:  mask = BUTTON_START;  break;
	case INPUT_SELECT: mask = BUTTON_SELECT; break;
	default: return;
	}

	if (pressed) {
		bus->input_state &= ~mask;
	} else {
		bus->input_state |= mask;
	}
}

