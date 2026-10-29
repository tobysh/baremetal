---
title: Keyboard Scancodes
summary: Read raw bytes from the PS/2 controller and decode scancode set 1 into characters, with shift, caps lock, ctrl, release codes and the 0xE0 extended prefix.
minutes: 50
difficulty: medium
tags: hardware, input, keyboard
id: osdev/scancodes
mode: harness
---
A keyboard does not send letters. It sends **scancodes**: numbers that name physical keys. The same key sends the same scancode whether you've typed `a`, `A` or `Ctrl+A`. Turning scancodes into characters is the kernel's job, and it is a small state machine. This lesson builds it, and reads the bytes from the real PS/2 controller.

## The PS/2 controller

The keyboard talks to an 8042 controller chip with two ports:

- port `0x64` **status** (read): bit 0 says the *output buffer is full*, a byte is waiting; bit 5 says the byte came from the mouse, not the keyboard.
- port `0x60` **data**: read the waiting byte (this also clears bit 0).

Polling is just: `if (inb(0x64) & 1) return inb(0x60);`. Later you will do the same inside an IRQ 1 handler instead.

## Make and break

Every key produces two scancodes in **set 1**, the one a PC starts with:

- a **make** code when it goes down, such as `0x1E` for the `A` key;
- a **break** code when it comes up: the same number with bit 7 set, `0x9E`.

Holding a key makes the keyboard repeat the make code. So the sequence `0x2A 0x1E 0x9E 0xAA` is: Shift down, A down, A up, Shift up, which types a capital `A`.

## The extended prefix

Keys added after the original 83 (arrows, right Ctrl, Home, ...) send `0xE0` first and then a code that can collide with an ordinary key's: `0xE0 0x48` is the Up arrow, while plain `0x48` is keypad 8. So the decoder is stateful: seeing `0xE0` just remembers "the next byte is extended" and produces no character.

## State you must track

- **Shift** has two keys (`0x2A` left, `0x36` right). If both are down and you let go of one, you are still shifting. Keep one bit per key: `shift` is a small bitmask, shifted when non-zero.
- **Ctrl** (`0x1D`, and `0xE0 0x1D` for the right one) is held, not toggled.
- **Caps Lock** (`0x3A`) toggles on each press and ignores the release.
- Caps lock only affects *letters*: with it on, `a` becomes `A` but `1` stays `1`. And shift reverses it: caps lock + shift + `a` gives `a`. So for letters, upper case when `shift XOR caps`; for everything else, when `shift`.
- **Ctrl + letter** produces a control code: `Ctrl+A` is 1, `Ctrl+C` is 3, `Ctrl+U` is 21, regardless of shift or caps.

## The tables

You only need codes `0x00` to `0x39`. The starter has two tables (indexes 0 to 57), one for the unshifted and one for the shifted characters (a `0` entry means "no character": Ctrl, Shift, Alt and the unused slots).

## Return values

`kbd_feed` returns an `int`:

- `0` if the byte produced no character (a prefix, a modifier, a release, an unknown key),
- the ASCII code for a character (`'\n'` for Enter, `'\b'` for Backspace, `'\t'` for Tab, `27` for Esc),
- one of `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT` (values above 255, so they can't be mistaken for characters) for the extended arrow keys.

## Debugging checklist

- **Every capital letter comes twice, or never:** the break codes aren't being filtered. Test bit 7 before indexing a table.
- **Arrow keys type digits:** the `0xE0` flag wasn't consulted before the table lookup.
- **Shift gets stuck:** you set it on make but cleared on break only for one of the two shift keys, or with one flag.
- **Caps lock + `1` gives `!`:** caps lock affects letters only.
- **Out of range reads:** codes of `0x3A` and above have no table entry.

## Your turn

The state is in a `KbdState`, zero-initialised, which the tests create for each scenario.
