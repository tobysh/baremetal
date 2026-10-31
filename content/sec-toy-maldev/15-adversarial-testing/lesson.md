---
title: Attack your own loader with generated inputs
summary: Generate malformed capsules from stable seeds, then shrink any failing input to a minimal reproduction.
minutes: 45
difficulty: hard
tags: security, toy-maldev, fuzzing, minimization
id: sec-toy-maldev/adversarial-testing
mode: harness
---

This course constructs **inert payload data** for an invented machine. Everything here is byte slices and pure functions. Nothing is executed, written to disk or sent anywhere. Everything stays inside the existing Go sandbox.

Hand-written test cases test what you thought of. Loaders fail on what you did not. **Generated** malformed inputs (fuzzing in the broad sense) are how defenders find those holes: take a valid capsule, damage it in many ways, and require that the loader either accepts it with a coherent result or rejects it, and never panics, hangs or reads out of bounds.

Three engineering habits make this useful instead of noisy.

**Stable seeds.** Random testing is only worth running in CI if a failure can be reproduced. Derive the pseudo-random stream from a seed, never from the clock, and derive the seed from the test's name and index so the same case always sees the same inputs and a different case does not share them.

**Mutations that reach deep.** A loader's first lines check the magic number and checksum. Blindly flipped bytes die there, and the interesting code below never runs. A good generator therefore also makes mutants whose header is consistent: change the body, then recompute the length and CRC32, so the damaged capsule gets past the outer checks. (This is the same reason CRC32 is no defence: it can be recomputed by anyone.)

**Minimization.** A failing input of 400 bytes is hard to reason about; the two bytes that matter are not. Delta-debugging style reduction repeatedly removes chunks of the input while the failure persists, ending with a result where removing any single byte makes the failure disappear (it is *1-minimal*; not necessarily the smallest possible, which is a harder problem).

The pseudo-random generator, the capsule format and its loader are provided. You build the three tools. Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
