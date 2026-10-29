Keep arrays of the success events. For each event, search backwards through the earlier ones for the closest match on `DST == SRC` within 600 seconds, and store its index in `parent[i]` (or -1). Mark `is_parent[p] = 1`.
===
A leaf is an event with `is_parent == 0` and a parent. Build the chain by following `parent` back to the root, collecting indexes in an array, then print them in reverse.
