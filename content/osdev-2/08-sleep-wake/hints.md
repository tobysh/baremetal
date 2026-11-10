A wait queue is a FIFO of task pointers. `head` is where the oldest waiter sits and `count` says how many follow it. Think about each operation as "where does the new entry go?" and "where does the oldest one come from?", with every index taken modulo the capacity.
===
The tail slot is not `count`: once some tasks have been woken, `head` has moved, so the free slot after the newest waiter is `(head + count) % 8`. Removing advances `head` the same way, `(head + 1) % 8`, and decrements `count`.
===
`sleep_on` has two reasons to refuse (task not `READY`, queue full). Check both **before** writing anything; if you change the task's state first and then discover the queue is full, the task is blocked with no queue to be woken from. `wake_one` on an empty queue returns `NULL` before reading `items[head]`.
===
The same ring shape for a different FIFO, a queue of pending scancodes:

```c
if (kq.count == 16) return -1;
kq.codes[(kq.head + kq.count) % 16] = code;
kq.count++;
...
uint8_t c = kq.codes[kq.head];
kq.head = (kq.head + 1) % 16;
kq.count--;
```
