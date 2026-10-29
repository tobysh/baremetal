Complete `BlockingQueue<T>` in the starter, built from a `Mutex<VecDeque<T>>` and a `Condvar`:

- `new()` creates an empty queue.
- `push(&self, item: T)` adds to the back and wakes one waiting consumer.
- `pop(&self) -> T` removes from the front, sleeping (not spinning) while the queue is empty.
- `pop_timeout(&self, timeout: Duration) -> Option<T>` waits at most `timeout` and returns `None` if nothing arrived.
- `len(&self) -> usize` is the current number of items.

Order is first in, first out.
