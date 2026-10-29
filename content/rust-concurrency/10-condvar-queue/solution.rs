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
        self.ready.notify_one();
    }

    fn pop(&self) -> T {
        let mut g = self.items.lock().unwrap();
        loop {
            if let Some(x) = g.pop_front() {
                return x;
            }
            g = self.ready.wait(g).unwrap();
        }
    }

    fn pop_timeout(&self, timeout: Duration) -> Option<T> {
        let g = self.items.lock().unwrap();
        let (mut g, _) = self.ready.wait_timeout_while(g, timeout, |q| q.is_empty()).unwrap();
        g.pop_front()
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
