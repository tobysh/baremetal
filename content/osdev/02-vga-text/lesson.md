---
title: The Screen: VGA Text Mode
summary: Write characters straight into video memory at 0xB8000, handle newlines, tabs, backspace and scrolling, and move the hardware cursor with port I/O.
minutes: 40
difficulty: easy
id: osdev/vga-text
mode: harness
---
Before graphics cards had frame buffers and drivers, every PC could do one thing with no setup at all: show an 80 by 25 grid of characters. Your machine still boots into this **VGA text mode**, and it is how most hobby kernels print their first line.

## Memory-mapped I/O

The serial port used **port I/O**: special `in`/`out` instructions. The screen uses the other classic technique, **memory-mapped I/O**: a range of ordinary addresses that is wired to a device instead of RAM. Video text memory lives at physical address `0xB8000`. A store to that address changes what is on the screen. No special instruction, just a pointer:

```c
volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
vga[0] = 0x0F48;   // white 'H' in the top-left corner
```

`volatile` tells the compiler that these reads and writes have effects it can't see, so it must not remove, merge or reorder them.

## One cell is 16 bits

The grid has 80 columns and 25 rows. Cell `(row, col)` is `vga[row * 80 + col]`. Each cell is a 16-bit value:

```
 15      12 11      8 7              0
+----------+---------+----------------+
|    bg    |   fg    |   character    |
+----------+---------+----------------+
   attribute byte (high)  ASCII (low)
```

The low byte is the character. The high byte is the **attribute**: foreground colour in the low four bits and background in the high four. Colours are `0` black, `1` blue, `2` green, `3` cyan, `4` red, `5` magenta, `6` brown, `7` light grey, up to `15` white. Light grey on black, attribute `0x07`, is the default.

## The cursor is separate hardware

The blinking cursor is not part of the cells. It is a position held in two registers of the VGA's CRT controller, reached through ports. Write a register *index* to port `0x3D4`, then write its value to `0x3D5`:

| Index | Register |
|-------|----------|
| `0x0F` | cursor position, low byte |
| `0x0E` | cursor position, high byte |

The position is `row * 80 + col`.

## Behaviour of `term_putc`

- Printable characters go into the cell at the cursor, then the cursor moves right. After column 79 it wraps to column 0 of the next row.
- `\n` moves to column 0 of the next row.
- `\r` moves to column 0 of the same row.
- `\b` moves one column left (not past column 0) and blanks that cell.
- `\t` moves to the next multiple of 8 columns. If that is column 80 or more, it wraps like a newline.
- When the cursor would go below row 24, **scroll**: copy rows 1 to 24 up one row, clear the last row, and keep the cursor on row 24.
- Blank cells are a space written with the *current colour*.
- After every `term_putc` the hardware cursor must match.

There's no `memmove` here, so write the copy loop yourself.

## Your turn

The starter gives you the pointer and the state variables. Fill in the functions. The screen isn't visible in the output panel, so rely on **Test**: it reads the real video memory and the real cursor registers.
