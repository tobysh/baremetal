Each address space is its own PML4, and the CPU walks whichever one is in `cr3`. The kernel lives in PML4 entry 0 (low memory, identity-mapped), so every new space must share that entry or the CPU cannot fetch its next instruction after a switch. User pages must carry the U bit at the leaf. Every table below entry 0 belongs to this space alone.
===
Five functions, and the starter's helpers do the hard walking: `map_page` builds missing tables, `leaf_pte` finds an entry. Check the lesson's notes on which flags go where. `aspace_destroy` is the only one that needs a walk of its own, one level of the tree at a time.
===
Typical mistakes: validating alignment or range after allocating (leaking a frame on the error path), not freeing the frame when `map_page` fails, `virt + len` overflowing so a huge `len` passes the range check, writing part of the data before discovering a later page is unmapped, copying through user addresses instead of through each frame's physical address (the target space may not be current), and freeing frames reachable via entry 0, which shows up as `bad_frees`.
===
Small example on a different task: to check a range for overflow safely, compare against the remaining room, `len > LIMIT - start`, rather than computing `start + len`.
