Write a small `fnv(h, bytes, n)` helper that continues from a given state, so the offset basis is only used when you start fresh.
===
Write the state to an 8 byte array with `b[j]=(uint8_t)(h>>(8*j))` for j from 0 to 7, then hash the array and the salt.
