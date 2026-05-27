#ifndef INPUT_H
#define INPUT_H

#include "./bus.h"

#define BUTTON_A        0x10
#define BUTTON_B        0x20
#define BUTTON_SELECT   0x40
#define BUTTON_START    0x80
#define BUTTON_RIGHT    0x01
#define BUTTON_LEFT     0x02
#define BUTTON_UP       0x04
#define BUTTON_DOWN     0x08

typedef enum {
	INPUT_A,
	INPUT_B,
	INPUT_SELECT,
	INPUT_START,
	INPUT_RIGHT,
	INPUT_LEFT,
	INPUT_UP,
	INPUT_DOWN,
	INPUT_QUIT
} InputKey;

typedef struct {
	InputKey key;
	bool pressed;
} InputEvent;

void input_handle(Bus* bus, InputKey key, bool pressed, bool* running);

#endif

