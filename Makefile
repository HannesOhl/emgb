CC := gcc

TARGET := xemgb

CFLAGS := -std=gnu23 -march=native -O3
CFLAGS += -Wall -Wextra
CFLAGS += -Wconversion -Wsign-conversion -Wfloat-equal
CFLAGS += -Wwrite-strings -Wstrict-prototypes -Wold-style-definition
CFLAGS += -Wswitch-default -Winit-self -Wundef
CFLAGS += -Wpointer-arith -Wcast-align
CFLAGS += -Wshadow
CFLAGS += -Wno-format-truncation

DFLAGS := -g -O0 -DDEBUG
SFLAGS := -fsanitize=address -fno-omit-frame-pointer

LFLAGS := lib/libSDL2.a
LFLAGS += -lm

SOURCE := src/main.c src/backend_sdl.c src/cpu.c src/bus.c src/ppu.c src/timer.c
OBJECT := $(SOURCE:.c=.o)

all: $(TARGET)
	@exec echo -e "\x1B[0;1;36m [ LD ]\x1B[0m xemgb"

$(TARGET): $(OBJECT)
	@$(CC) $(OBJECT) $(LFLAGS) -o $@

%.o: %.c
	@exec echo -e "\x1B[0;1;35m [ CC ]\x1B[0m $@"
	@$(CC) $(CFLAGS) -c $< -o $@

# debug build
debug: CFLAGS := $(CFLAGS) $(DFLAGS) $(SFLAGS)
debug: LFLAGS := $(LFLAGS) $(SFLAGS)
debug: rebuild

clean:
	@rm -f $(OBJECT) && rm -f $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild debug
