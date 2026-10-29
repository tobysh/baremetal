use std::sync::mpsc;
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};

type Job = Box<dyn FnOnce() + Send + 'static>;

struct ThreadPool {
    size: usize,
}

impl ThreadPool {
    fn new(size: usize) -> ThreadPool {
        assert!(size > 0, "a pool needs at least one worker");
        ThreadPool { size }
    }

    // Wrong: runs the job right here, on the caller's thread.
    fn execute<F>(&self, f: F)
    where
        F: FnOnce() + Send + 'static,
    {
        f();
    }

    fn size(&self) -> usize {
        self.size
    }
}

// Try it with Run.
fn main() {
    let pool = ThreadPool::new(3);
    for i in 0..5 {
        pool.execute(move || println!("job {i} on {:?}", thread::current().id()));
    }
}
