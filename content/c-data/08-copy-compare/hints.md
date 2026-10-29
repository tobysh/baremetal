`str_equal` and `str_compare` share a loop: advance `i` while `a[i] != '\0' && a[i] == b[i]`. Afterwards, `a[i]` and `b[i]` are either both `'\0'` (equal) or the first characters that differ.
===
Equal means `a[i] == b[i]` after that loop. For the sign of the comparison, subtract: `(unsigned char)a[i] - (unsigned char)b[i]`. For `str_copy`, copy until `src[i]` is `'\0'` and then write the terminator into `dst[i]` yourself. For `str_append`, first find `end`, the index of dst's terminator, then copy `src` to `dst[end + i]`.
===
```c
void str_append(char dst[], char src[]) {
    int end = 0;
    while (dst[end] != '\0') end++;
    int i = 0;
    while (src[i] != '\0') {
        dst[end + i] = src[i];
        i++;
    }
    dst[end + i] = '\0';
}
```
