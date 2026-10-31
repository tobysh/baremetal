Best fit needs a variable `best` initialised to `NULL`. In the loop, count the step first, skip unusable blocks, then update `best` when `best == NULL || b->size < best->size`.
===
First fit returns inside the loop as soon as it finds a block; best fit returns after the loop. Increment `search_steps` at the top of each iteration, before any `continue`.
===
For `run_workload`: `static _Alignas(16) unsigned char buf[2048]; void *ptr[32] = {0};`, call `heap_init(buf, sizeof buf); heap_set_policy(p);`, then loop over the ops.
===
Read the final counters with `heap_stats(&st)`: `st.failed_allocs`, `st.search_steps`, `st.largest_free`, `st.free_blocks`.
