CC := cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS :=
LDLIBS := $(shell sdl2-config --libs)
SDL_CFLAGS := $(shell sdl2-config --cflags)

TARGET := build/pixel
SRCS := src/main.c src/framebuffer.c src/brush.c src/export.c src/history.c
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
