# CHIP-8 Emulator

A CHIP-8 interpreter written in C with SDL2. It runs classic CHIP-8 games like Pong and Space Invaders, with sound and a retro phosphor-style display.


## Features

- Full CHIP-8 instruction set (fetch, decode, execute loop in a single `cycle()` function)
- CPU speed decoupled from the display: configurable instructions per second (default 700)
- Delay and sound timers ticking at exactly 60 Hz, independent of CPU speed
- Square-wave audio (440 Hz) while the sound timer is active, with a short ramp to avoid clicks
- Phosphor-style rendering: pixels brighten quickly and fade slowly, which reduces flicker from XOR sprite drawing
- Runtime hotkeys for pause, reset and speed control

## Requirements

- A C compiler (gcc or clang)
- SDL2 development libraries

Install SDL2:

```bash
# Debian / Ubuntu
sudo apt install libsdl2-dev

# Fedora
sudo dnf install SDL2-devel

# macOS (Homebrew)
brew install sdl2
```

## Build

```bash
gcc chip.c -o chip $(sdl2-config --cflags --libs) -lm
```

## Run

```bash
./chip path/to/rom.ch8
```

Example:

```bash
./chip Pong.ch8
```

## Controls

CHIP-8 uses a 16-key hex keypad, mapped to the left side of a QWERTY keyboard:

```
CHIP-8 keypad        Keyboard
+---+---+---+---+    +---+---+---+---+
| 1 | 2 | 3 | C |    | 1 | 2 | 3 | 4 |
| 4 | 5 | 6 | D |    | Q | W | E | R |
| 7 | 8 | 9 | E |    | A | S | D | F |
| A | 0 | B | F |    | Z | X | C | V |
+---+---+---+---+    +---+---+---+---+
```

Emulator hotkeys:

| Key | Action |
|-----|--------|
| `P` | Pause / resume |
| `Backspace` | Reset the current ROM |
| `+` / `-` | Increase / decrease CPU speed (100 to 5000 IPS, steps of 100) |
| `Esc` | Quit |

## How it works

- **Memory:** 4 KB, with ROMs loaded at `0x200` and the built-in font at `0x50`.
- **Registers:** sixteen 8-bit registers `V0`-`VF` (`VF` is the carry/collision flag), a 16-bit index register `I`, and a 16-level call stack.
- **Display:** 64x32 monochrome. Sprites are XOR-drawn, and `VF` is set on collision.
- **Timing:** each frame, elapsed real time is converted into a number of CPU cycles and a number of 60 Hz timer ticks, so games run at the same speed regardless of frame rate.

## Known limitations

- Original CHIP-8 only: no SUPER-CHIP or XO-CHIP extensions.
- Unimplemented opcodes are printed to the terminal rather than halting the emulator.

## Possible future work

- Command-line options for window scale, speed and quirks
- Unit tests for the arithmetic opcodes and a headless CPU core
- Disassembler / debugger view
- Save states
- SUPER-CHIP support

## ROMs and credits

Included ROMs are for testing. They belong to their original authors and credit goes to them.

- Pong, Space Invaders, Maze (David Winter and others): classic public-domain-era CHIP-8 games
- `1-chip8-logo.ch8`, `2-ibm-logo.ch8`: from the Timendo CHIP-8 test suite

Resources used while building this:

- [Cowgod's CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
- [Tobias V. Langhoff's guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)
