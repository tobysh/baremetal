Identify the state register, its starting value and the three steps inside the loop in order.
===
In C a rotate left by 5 of a 32-bit unsigned value is `(h << 5) | (h >> 27)`. The multiply and the starting constant fit in `uint32_t`.
===
After the loop, return `h ^ (h >> 15)`. For `n == 0` the loop does not run, so you get that step applied to the starting constant.
