`parse_int`: check the first character for `-` or `+` and remember the sign. Then `while (s[i] >= '0' && s[i] <= '9')` do `value = value * 10 + (s[i] - '0');`. Multiply by the sign at the end.
===
`int_to_text`: handle `n < 0` by remembering it and negating `n`. If `n == 0`, write `'0'`. Otherwise repeatedly write `'0' + n % 10` and divide `n` by 10. That produces the digits backwards, so add the `'-'` last, put the terminator at `out[len]`, and then reverse `out[0..len-1]` with two indexes.
===
```c
int left = 0;
int right = len - 1;
while (left < right) {
    char tmp = out[left];
    out[left] = out[right];
    out[right] = tmp;
    left++;
    right--;
}
```
