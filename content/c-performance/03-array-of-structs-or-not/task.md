`advance_x_soa()` must do the work of `advance_x_aos()` on the SoA arrays `sx` and `svx`: for each of the 128 particles, `sx[i] += svx[i]`.

- Use `ld(&sx[i])`, `ld(&svx[i])` and `st(&sx[i], ...)` so the model sees the traffic. `svx` is never changed.
- One call must cost at most **16 cache misses** from a cold cache, and exactly 3 accesses per particle.
- The particle array `ps` is not touched by the SoA step. `soa_load()` (setup, not modelled) is already written.

Several initial states (including negative values) and several steps in a row are tried and compared with a plain reference. Do not edit the cache block or the AoS version.
