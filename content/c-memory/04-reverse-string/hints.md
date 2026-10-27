Find the length first. Then use two indices: `left` starting at 0 and `right` starting at the last real character, `len - 1`.
===
Loop while `left < right`. In each round swap `s[left]` and `s[right]` using a temporary `char`, then move `left` forward and `right` backward. Be careful: for the empty string, `len - 1` would wrap around, so return early when `len < 2`.
===
```c
size_t len = strlen(s);
if (len < 2) return;
size_t left = 0, right = len - 1;
while (left < right) {
    char tmp = s[left];
    // finish the swap, then move both indices
}
```
