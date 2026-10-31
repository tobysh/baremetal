Sign extension: mask to 12 bits, then `(v ^ 0x800) - 0x800`. Do it in `int32_t` so the subtraction can go negative.
===
Convert the magnitude, then restore the sign: `mag = v < 0 ? -v : v; mc = (mag * 125 + 1) / 2;`. This rounds halves away from zero.
===
Poll loop: read STATUS once per iteration and test FAULT before DONE. Check the clock only after an unsuccessful read.
