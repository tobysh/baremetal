Both policies share one loop. Increment `steps` as the *first* statement of each iteration, before any `continue`, so used blocks and too-small blocks are counted as well.
===
After counting, skip blocks that cannot serve the request: `if (!b->is_free || b->size < need) continue;`. For `FIRST_FIT` return `b` right here. That early return is the only difference in cost between the policies.
===
For `BEST_FIT` keep a `Block *best = NULL` outside the loop. Replace it only when `!best || b->size < best->size`. The strict `<` is what makes the lowest address win ties. Return `best` after the loop, which is also the `NULL` "nothing fits" case.
===
For `run_workload`, declare `static _Alignas(16) unsigned char buf[2048];` and `void *ptr[32] = {0};`, call `heap_init` and then `heap_set_policy(p)` (init resets the policy to first fit, so the order matters). Read the results with `HeapStats st; heap_stats(&st);` and copy `failed_allocs`, `search_steps`, `largest_free`, `free_blocks`.
