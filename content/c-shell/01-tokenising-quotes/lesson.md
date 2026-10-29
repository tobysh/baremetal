---
title: Tokenising with Quotes
summary: Turn a command line into an argv array, honouring single quotes, double quotes and backslashes, the way a real shell does before it runs anything.
minutes: 30
difficulty: medium
tags: parsing, strings, shell
id: c-shell/tokenising-quotes
mode: harness
---
Every shell starts the same way: read a line, then split it into **words**. In `grep "hello world" notes.txt` there are three words, not four, because the quotes keep `hello world` together. The finished array is called `argv`, and it is exactly what a program receives in its own `main(int argc, char *argv[])`.

## The rules

- Spaces and tabs separate words, unless they are quoted.
- `'...'`: everything inside is literal, no exceptions.
- `"..."`: everything is literal except that `\"` is a quote and `\\` is a backslash.
- Outside quotes, a backslash makes the next character literal, so `a\ b` is one word `a b`.
- Quotes can sit in the middle of a word: `ab"c d"ef` is the single word `abc def`: the pieces are glued together.
- An empty pair of quotes, `""`, still makes a word (an empty one).
- A quote that is never closed is an error.

## Writing the words into a buffer

Because the quotes disappear, a word is usually shorter than the text that produced it, so you can write the result into one scratch buffer and point `argv[i]` into it. Keep a write position `o`, copy characters as you decide they are literal, and put a `'\0'` at the end of each word:

```c
argv[n] = buf + o;
/* ... buf[o++] = ch; for every literal character ... */
buf[o++] = '\0';
n++;
```

Always check that `o` stays inside `buf_size` before every write. Shells are fed text by users and by scripts, so never trust the length.

The loop shape is the cursor pattern you met in *Tokenising Text by Hand*, with one extra twist: inside a word you are in one of three modes (plain, single-quoted, double-quoted) and you switch mode when you meet a quote character.

## Your turn

Write `tokenize`. The next lessons will feed its output to `execv`.
