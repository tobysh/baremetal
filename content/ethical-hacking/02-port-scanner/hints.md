Loop over ports. For each one keep an attempt counter from 0 to 2 and stop the inner loop as soon as the probe returns something other than `PORT_TIMEOUT`.
===
After the inner loop, if the last answer was still `PORT_TIMEOUT`, increment `*filtered`. If it was `PORT_OPEN` and `count < cap`, store the port and increment the count. Remember to set `*filtered = 0` after the range check.
===
```c
for (int p = lo; p <= hi; p++) {
    int r = PORT_TIMEOUT;
    for (int a = 0; a < 3 && r == PORT_TIMEOUT; a++) r = probe(ctx, p);
    if (r == PORT_TIMEOUT) (*filtered)++;
    else if (r == PORT_OPEN && n < cap) open[n++] = p;
}
```
