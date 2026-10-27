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

### Building a cell step by step

Combine a white-on-blue `A` (white is 15, blue is 1):

```
attribute  = fg | (bg << 4)     = 15 | (1 << 4)   = 0x1F
cell       = (attribute << 8) | ch = (0x1F << 8) | 0x41 = 0x1F41
```

So the helper is one line, but every piece matters. The character is an 8-bit value, so cast it to `uint8_t` before widening. A plain `char` that is negative (for characters above 127) would be sign-extended and set the upper bits, corrupting the colour. The same care applies in reverse: when you mask or shift the attribute make sure the arithmetic happens on an unsigned type.

The 16 colours are the same for foreground and background in principle, but in the default configuration the top bit of the background nibble means **blink** rather than a bright background, so on real hardware the eight bright colours are normally only available for the foreground. The tests use ordinary values, so you do not need to special-case this.

### Why `0xB8000`?

The original VGA card had 128 KiB of its own memory, which the PC mapped into the address range `0xA0000` to `0xBFFFF`, just below 1 MiB. Graphics modes used `0xA0000`, and colour text mode uses `0xB8000`. A text screen is 80 x 25 = 2000 cells of 2 bytes, so it occupies exactly 4000 bytes starting there. The hardware continually reads those bytes, looks up each character in a font stored on the card, and draws it, so changing the memory is changing the picture on the next refresh. That is why the guide calls it memory-mapped: the "device" is just a region of the address space.

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

### Moving the cursor, in detail

The cursor position is one 16-bit number but the controller exposes it as two 8-bit registers, so you split it:

```c
uint16_t pos = row * 80 + col;
outb(0x3D4, 0x0F);            // select "cursor low"
outb(0x3D5, pos & 0xFF);      // write the low byte
outb(0x3D4, 0x0E);            // select "cursor high"
outb(0x3D5, pos >> 8);        // write the high byte
```

This **index/data pair** is a common way for a chip to expose many registers through only two ports: one port picks the register, the other reads or writes it. You will see the same trick with the CMOS clock, the PCI configuration space and others.

### Scrolling, worked through

When the cursor would move past row 24 there is no row 25 to put it on, so the whole screen moves up one line to make room:

```
before:   row 0   line A          after:   row 0   line B
          row 1   line B                   row 1   line C
          ...                              ...
          row 24  line Z                   row 23  line Z
                  (cursor wants row 25)    row 24  (blank, cursor here)
```

Concretely, copy cells `80 .. 1999` down to `0 .. 1919` (that is `80 * 24 = 1920` cells), fill cells `1920 .. 1999` with blanks in the current colour, and put the cursor at row 24. Copy in **increasing** order: each cell is read before anything overwrites it, because the destination is always earlier in memory than the source. (Copying overlapping memory the other way round is a classic bug, and is exactly why `memmove` is a separate function from `memcpy`.)

### Each control character as arithmetic

| Character | New position |
|---|---|
| `\n` | `col = 0`, `row + 1` |
| `\r` | `col = 0` |
| `\b` | if `col > 0` then `col - 1`, and blank that cell |
| `\t` | `col = (col + 8) & ~7`, i.e. the next multiple of 8 |
| printable | write the cell, then `col + 1`; if `col` reaches 80 wrap to `col = 0`, `row + 1` |

The tab formula works because 8 is a power of two: adding 8 and clearing the low three bits rounds up to the next multiple of 8, and from column 8 it gives 16, not 8, so a tab always advances. Every case that increases `row` then needs the same "did we pass row 24?" check, so it is worth putting scrolling in one helper that all of them call.

## Your turn

The starter gives you the pointer and the state variables. Fill in the functions. The screen isn't visible in the output panel, so rely on **Test**: it reads the real video memory and the real cursor registers.
