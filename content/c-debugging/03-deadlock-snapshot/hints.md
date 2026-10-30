Write a helper that answers "which array index holds lock L?" by scanning every thread's `held` list. Return -1 if nobody does.
===
Each thread has one outgoing arrow, to `owner(t.waiting)`. Walk from a thread; if you hit -1 the chain ends and there is no cycle through this start.
===
To land on a cycle, walk `n` steps from each start (a chain of non-cycle threads is shorter than `n`). If you never hit -1, you are now on a cycle. Walk once around it to collect ids, then rotate so the smallest id comes first.
===
For `suggest_order`, call `find_deadlock`, then map each thread id in the cycle back to its `waiting` lock and sort the small array.
