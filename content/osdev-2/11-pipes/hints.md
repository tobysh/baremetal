There are three different "nothing happened" answers here, and they mean different things: 0 bytes moved because none were asked for or none fitted, -1 because the pipe is empty but more may come, and 0 at end of file. Write down which state of `count` and `closed` leads to each before writing code.
===
`pipe_write` has one early exit: a closed pipe returns -1 before anything else, even for `n == 0`. After that it is a loop that stops either when all `n` bytes are copied or when `count` reaches 16; return how many it copied. The free slot is `(head + count) % 16`.
===
`pipe_read` checks in this order: `n == 0` returns 0; an empty pipe returns `closed ? 0 : -1`; otherwise loop while bytes are wanted and `count > 0`, taking `data[head]` and advancing `head` modulo 16. Do not test `closed` before `count`, or buffered data is lost at close.
===
The same "try again versus finished" split for a different producer, a keyboard line buffer:

```c
if (line_len == 0)
    return keyboard_unplugged ? 0 : -1;   /* finished, or nothing yet */
```
