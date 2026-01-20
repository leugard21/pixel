# Pixel

A lightweight pixel art editor built with C and SDL2.

## Features

- **Multiple Drawing Tools**
  - Brush tool for freehand drawing
  - Line tool for straight lines
  - Rectangle tool (outline and filled)
  - Circle tool (outline and filled)

- **Canvas Controls**
  - Pan and zoom support
  - Grid overlay for precise pixel placement
  - Adjustable brush size (1-64 pixels)

- **Undo/Redo System**
  - 32-level undo/redo history
  - Full canvas state preservation

- **Color Palette**
  - 8 preset colors accessible via number keys
  - Color picker (Alt+Click)
  - Real-time color preview in HUD

- **Export**
  - Save canvas as BMP image
  - Timestamped exports to `exports/` directory

## Dependencies

- SDL2
- SDL2_ttf
- A C11-compatible compiler

### Installing Dependencies

**Ubuntu/Debian:**

```bash
sudo apt-get install libsdl2-dev libsdl2-ttf-dev
```

**Fedora:**

```bash
sudo dnf install SDL2-devel SDL2_ttf-devel
```

**macOS (Homebrew):**

```bash
brew install sdl2 sdl2_ttf
```

## Building

```bash
make
```

## Running

```bash
make run
# or
./build/pixel
```

## Keyboard Shortcuts

### Tools

- **F1** - Brush tool
- **F2** - Line tool
- **F3** - Rectangle tool
- **F4** - Circle tool

### Drawing

- **Left Click** - Draw with selected tool
- **Alt + Left Click** - Pick color from canvas
- **[ / ]** - Decrease/Increase brush size

### View Controls

- **Space + Left Click** or **Middle Click** - Pan canvas
- **Mouse Wheel** - Zoom in/out
- **R** - Reset view (zoom 100%, center canvas)
- **G** - Toggle grid overlay

### Canvas

- **C** - Clear canvas
- **F** - Toggle fill mode (for shapes)
- **H** - Toggle HUD visibility

### File Operations

- **Ctrl+S** - Save canvas as BMP
- **Ctrl+Z** - Undo
- **Ctrl+Y** - Redo

### Color Palette

- **1** - White
- **2** - Black
- **3** - Red
- **4** - Green
- **5** - Blue
- **6** - Yellow
- **7** - Magenta
- **8** - Cyan

### Application

- **ESC** - Quit

## License

This project is open source. Feel free to use and modify as needed.
