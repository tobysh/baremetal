use std::collections::VecDeque;
use std::sync::{Condvar, Mutex};
use std::time::Duration;

struct BlockingQueue<T> {
    items: Mutex<VecDeque<T>>,
    ready: Condvar,
}

impl<T> BlockingQueue<T> {
    fn new() -> BlockingQueue<T> {
        BlockingQueue { items: Mutex::new(VecDeque::new()), ready: Condvar::new() }
    }

    fn push(&self, item: T) {
        self.items.lock().unwrap().push_back(item);
    }

    // Wrong: panics instead of sleeping when the queue is empty.
    fn pop(&self) -> T {
        self.items.lock().unwrap().pop_front().expect("queue is empty")
    }

    fn pop_timeout(&self, timeout: Duration) -> Option<T> {
        self.items.lock().unwrap().pop_front()
    }

    fn len(&self) -> usize {
        self.items.lock().unwrap().len()
    }
}

// Try it with Run.
fn main() {
    let q = BlockingQueue::new();
    q.push(1);
    q.push(2);
    println!("{} {}", q.pop(), q.len());
}
