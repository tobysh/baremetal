Remember whether any thread was enabled while looping over the threads.
===
After the loop, if the state is not `all_done` and no thread was enabled, set `verdict = V_DEADLOCK`, `len = depth` and return 1.
===
The `all_done` branch comes first, otherwise a finished program looks like "nothing enabled".
