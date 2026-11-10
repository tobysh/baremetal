Write a program that simulates a 32-bit flag register. It starts at 0. The input is any number of commands, one per line, until the input ends. Bits are numbered 0 to 31 from the least significant end.

- `set N` turns bit `N` on.
- `clear N` turns bit `N` off.
- `toggle N` flips bit `N`.
- `test N` prints `1` if bit `N` is on, otherwise `0`.
- `show` prints the register as `flags: 0x` followed by eight uppercase hexadecimal digits, then the decimal value in brackets.

For example, the commands `set 3`, `set 0`, `show`, `toggle 3`, `test 3`, `test 0`, `show` produce:

```
flags: 0x00000009 (9)
0
1
flags: 0x00000001 (1)
```

Every bit position from 0 to 31 is tried, on several different series. Start from the empty file: choose your own helper functions and `main`.
