A `thread_state` has an `id`, the locks it holds (`held[0..nheld-1]`) and the lock it waits for (`waiting`, or `-1` if it is not waiting). Every lock is held by at most one thread. Ids are arbitrary integers, not array indices.

**`int find_deadlock(const thread_state *t, int n, int *out)`**

- Follow "waits for a lock held by" arrows and find a cycle.
- Write the **thread ids** of the cycle into `out` and return its length. Start with the smallest id in the cycle, then follow the arrows (each next thread holds the lock the previous one waits for).
- A thread waiting for a lock it holds itself is a cycle of length 1.
- Return `0` and leave `out` alone if there is no cycle. A waiting chain that ends at a running thread, or a wait for a lock nobody holds, is **not** a cycle. Threads that only wait on a cycle, and bystanders, are not part of the answer.
- If there are several separate cycles, report the one that contains the smallest thread id.
- `out` has room for `n` entries.

**`int suggest_order(const thread_state *t, int n, int *locks)`**

- If there is a deadlock (as found by `find_deadlock`), write the ids of the locks the cycle's threads are waiting for into `locks`, sorted from smallest to largest (the order every thread should take them in), and return how many.
- Return `0` if there is no deadlock. `locks` has room for `n` entries.

Many different snapshots are tried: no deadlock, two and three threads in a cycle, chains that are not cycles, cycles with bystanders, and different id numbering. Do not rely on thread ids being small or ordered in the array.
