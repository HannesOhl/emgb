CC      := gcc
TARGET  := xemgb
BUILD   := build
BACKEND ?= console

CFLAGS := -std=gnu23 -march=native -O3
CFLAGS += -Wall -Wextra
CFLAGS += -Wconversion -Wsign-conversion -Wfloat-equal
CFLAGS += -Wwrite-strings -Wstrict-prototypes -Wold-style-definition
CFLAGS += -Wswitch-default -Winit-self -Wundef
CFLAGS += -Wpointer-arith -Wcast-align
CFLAGS += -Wshadow
CFLAGS += -Wno-format-truncation

DFLAGS  := -g -O0 -DDEBUG
SFLAGS  := -fsanitize=address -fno-omit-frame-pointer

CORE_SRC := src/main.c src/bus.c src/cpu.c src/ppu.c src/tmr.c src/input.c

ifeq ($(BACKEND),sdl2)
	BACKEND_SRC := backend_sdl2/backend.c
	LFLAGS      := backend_sdl2/libSDL2.a -lm
	VPATH       := src backend_sdl2
else ifeq ($(BACKEND),console)
	BACKEND_SRC := backend_console/backend.c
	LFLAGS      := -lm
	VPATH       := src backend_console
else
	$(error Unknown BACKEND '$(BACKEND)' (use console or sdl2))
endif

SRC  := $(CORE_SRC) $(BACKEND_SRC)
OBJ  := $(addprefix $(BUILD)/,$(notdir $(SRC:.c=.o)))

vpath %.c $(VPATH)

all: $(TARGET)

$(TARGET): $(OBJ)
	@$(CC) $^ $(LFLAGS) -o $@
	@echo -e "\x1B[0;1;36m [ LD ]\x1B[0m $@"

$(BUILD)/%.o: %.c
	@mkdir -p $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "\x1B[0;1;35m [ CC ]\x1B[0m $@"

debug: CFLAGS += $(DFLAGS) $(SFLAGS)
debug: LFLAGS += $(SFLAGS)
debug: rebuild

sdl2:
	@$(MAKE) --no-print-directory clean BACKEND=sdl2
	@$(MAKE) --no-print-directory BACKEND=sdl2

console:
	@$(MAKE) --no-print-directory clean BACKEND=console
	@$(MAKE) --no-print-directory BACKEND=console

clean:
	@rm -rf $(BUILD) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild debug sdl2 console
