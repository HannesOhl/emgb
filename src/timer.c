#include "../inc/timer.h"
#include "../inc/bus.h"

static uint16_t timer_get_threshold(uint8_t tac) {
	switch (tac & 0x03) {
		case 0: return 1024;
		case 1: return 16;
		case 2: return 64;
		case 3: return 256;
		default: return 0;
	}
	return 1024;
}

void tmr_init(Tmr* tmr, Bus* bus) {
	tmr->div_counter  = 0;
	tmr->tima_counter = 0;
	bus_write(bus, IO_TIMA, 0);
	bus_write(bus, IO_TMA , 0);
	bus_write(bus, IO_TAC , 0);
}

void tmr_step(Tmr* tmr, Bus* bus, uint32_t cycles) {

	tmr->div_counter += (uint16_t) cycles;

	bus_write(bus, IO_DIV, (uint8_t) (tmr->div_counter >> 8));

	// disable timer
	if (!(bus_read(bus, IO_TAC) & 0x04)) return;

	tmr->tima_counter += (uint16_t) cycles;

	uint16_t threshold = timer_get_threshold(bus_read(bus, IO_TAC));

	while (tmr->tima_counter >= threshold) {
		tmr->tima_counter -= threshold;

		if (bus_read(bus, IO_TIMA) == 0xFF) {
			bus_write(bus, IO_TIMA, bus_read(bus, IO_TMA));

			uint8_t iflag = bus_read(bus, IO_IF);
			bus_write(bus, IO_IF, iflag | 0x04);
		} else {
			uint8_t tima = bus_read(bus, IO_TIMA) + 1;
			bus_write(bus, IO_TIMA, tima);
		}
	}

	// sync registers to bus
	uint8_t tima = bus_read(bus, IO_TIMA);
	uint8_t tma  = bus_read(bus, IO_TMA);
	uint8_t tac  = bus_read(bus, IO_TAC);
	bus_write(bus, IO_TIMA, tima);
	bus_write(bus, IO_TMA , tma);
	bus_write(bus, IO_TAC , tac);
}

