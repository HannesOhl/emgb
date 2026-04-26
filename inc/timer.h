#ifndef TIMER_H
#define TIMER_H

#include "./bus.h"

#include <stdint.h>

typedef struct {

	uint16_t div_counter;
	uint16_t tima_counter;
} Tmr;

void tmr_init(Tmr* tmr, Bus* bus);
void tmr_step(Tmr* tmr, Bus* bus, uint32_t cycles);

#endif

