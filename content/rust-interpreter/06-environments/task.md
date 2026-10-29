Implement `Env::new`, `child`, `define`, `get`, `assign` and `depth`.

A fresh environment has an empty global frame and fresh output buffer. A child has its own empty local map, a parent pointing at the original frame, and the same output buffer. Cloning preserves shared state.

`define` inserts or overwrites locally. `get` clones the nearest value. `assign` overwrites the nearest existing binding and reports success; it returns false without creating a binding when absent. `depth` counts this frame and every parent, so a global has depth 1.

The hidden tests use independent roots, multiple children, shadowing, mutation through clones and shared output.
