For `count_bits`, either test the lowest bit and shift right 32 times, or repeatedly clear the lowest 1 bit with `x &= x - 1` and count how many times you could. A power of two has exactly one bit set.
===
`is_power_of_two`: rule out 0 first, then check that `x & (x - 1)` is 0. For `swap_endian`, pull out each byte with `(x >> n) & 0xFF` and place it in the mirrored position with `<< m`, then OR the four pieces together.
===
```c
return ((x >> 24) & 0x000000FFu)
     | ((x >>  8) & 0x0000FF00u)
     | ((x <<  8) & 0x00FF0000u)
     | ((x << 24) & 0xFF000000u);
```
Check that you understand why each mask lines up before pasting; then try writing `count_bits` and `is_power_of_two` yourself.
