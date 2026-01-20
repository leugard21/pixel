CC := cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS :=
SDL_CFLAGS := $(shell sdl2-config --cflags) $(shell pkg-config --cflags SDL2_ttf)
LDLIBS := $(shell sdl2-config --libs) $(shell pkg-config --libs SDL2_ttf) -lm

TARGET := build/pixel
SRCS := src/main.c src/framebuffer.c src/brush.c src/export.c src/history.c src/ui.c src/ui_components.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean run help install uninstall

all: $(TARGET)

$(TARGET): $(OBJS) | build
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf build

run: all
	./$(TARGET)

help:
	@echo "Pixel - Lightweight Pixel Art Editor"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build the project (default)"
	@echo "  make all      - Build the project"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Build and run the application"
	@echo "  make install  - Install to /usr/local/bin (requires sudo)"
	@echo "  make uninstall- Uninstall from /usr/local/bin (requires sudo)"
	@echo "  make help     - Show this help message"

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/pixel
	@echo "Installed to /usr/local/bin/pixel"

uninstall:
	rm -f /usr/local/bin/pixel
	@echo "Uninstalled from /usr/local/bin/pixel"

