---
title: "Review: A Line Editor"
summary: Combine scancode decoding, a bounded buffer and serial echo into the first piece of a shell, a line editor with backspace, Ctrl+U, Ctrl+C and a length limit.
minutes: 40
difficulty: medium
kind: review
tags: input, keyboard, serial, strings
id: osdev/review-line-editor
mode: harness
---
This is a consolidation exercise. You already have all the pieces; this lesson wires them together into the first part of a shell: a **line editor**. When you type at a prompt, the kernel collects characters into a line, lets you fix mistakes, echoes what you type, and hands over the finished line when you press Enter.

The starter provides the working serial driver (you wrote one in the first lesson) and the scancode decoder `kbd_feed` from the keyboard lesson. You write the editor.

## What a terminal editor does

For every decoded character:

| Input | Effect on the buffer | Echo to the serial port |
|---|---|---|
| a printable character (32 to 126) | appended if there is room | the character |
| Backspace (`'\b'`) | removes the last character, if any | `"\b \b"` (back, blank, back) |
| `Ctrl+U` (21) | removes the whole line | one `"\b \b"` per removed character |
| `Ctrl+C` (3) | removes the whole line | `"^C\r\n"` |
| Enter (`'\n'`) | finishes the line | `"\r\n"` |
| anything else (arrow keys, Tab, ...) | nothing | nothing |

When the buffer is full (`LINE_MAX - 1` characters, one slot is kept for the terminating `'\0'`), extra printable characters are **ignored silently**: not stored and not echoed, so the screen always matches the buffer. Backspace on an empty line does nothing and echoes nothing, for the same reason.

The echo of a backspace is three bytes because a terminal's backspace only moves the cursor left. To *erase* a character you move left, overwrite with a space and move left again.

## The finished line

After Enter, the line is **done**: the text stays in the editor until the caller takes it with `line_take`, which copies it out (truncating to the output size) and starts a new empty line. If more characters arrive while a finished line is still waiting, the editor starts a fresh line first, so a forgotten `line_take` can't glue two commands together.

## Two layers

Split the work in two so each layer can be tested alone:

- `line_feed_char` handles one decoded character: pure editing and echo.
- `line_feed_scancode` decodes one raw byte with `kbd_feed` and passes any character on. A raw byte that produces nothing, or an arrow key (the `KEY_*` values are above 255), must not reach `line_feed_char` as a character.

Both return 1 on the call that completes a line, and 0 otherwise.

## Debugging checklist

- **The screen shows garbage after deleting:** backspace echoes `'\b'`, `' '`, `'\b'`, in this order.
- **The buffer overflows with a long line:** keep `len < LINE_MAX - 1` before storing.
- **Characters keep being echoed on a full line:** the echo belongs inside the "was stored" branch.
- **Backspace eats the prompt:** nothing should be echoed when `len` is already 0.
- **`Shift` gets echoed or stored:** the decoder returns 0 for it, so `line_feed_scancode` must skip zero values.

## Your turn

The tests put the UART into loopback so that they can read exactly what your editor echoed.
