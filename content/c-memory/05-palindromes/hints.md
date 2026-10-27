Use two indices, `left` at the start and `right` at the last character. If `s[left]` differs from `s[right]`, return 0 right away. If the loop finishes, return 1.
===
Find the length with `strlen`, and return 1 immediately if it is less than 2, so `len - 1` never wraps. For the case-insensitive version, compare `tolower((unsigned char)s[left])` with `tolower((unsigned char)s[right])`.
===
```c
size_t len = strlen(s);
if (len < 2) return 1;
size_t left = 0, right = len - 1;
while (left < right) {
    if (s[left] != s[right]) {
        return 0;
    }
    left++;
    right--;
}
return 1;
```
