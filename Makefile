CC := cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS :=
SDL_CFLAGS := $(shell sdl2-config --cflags) $(shell pkg-config --cflags SDL2_ttf)
LDLIBS := $(shell sdl2-config --libs) $(shell pkg-config --libs SDL2_ttf) -lm

TARGET := build/pixel
SRCS := src/main.c src/framebuffer.c src/brush.c src/export.c src/history.c src/ui.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | build
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET)
