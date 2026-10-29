use std::sync::mpsc;
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};

type Job = Box<dyn FnOnce() + Send + 'static>;

struct ThreadPool {
    workers: Vec<JoinHandle<()>>,
    sender: Option<mpsc::Sender<Job>>,
}

impl ThreadPool {
    fn new(size: usize) -> ThreadPool {
        assert!(size > 0, "a pool needs at least one worker");
        let (tx, rx) = mpsc::channel::<Job>();
        let rx = Arc::new(Mutex::new(rx));
        let workers = (0..size)
            .map(|_| {
                let rx = Arc::clone(&rx);
                thread::spawn(move || loop {
                    let job = rx.lock().unwrap().recv();
                    match job {
                        Ok(job) => job(),
                        Err(_) => break,
                    }
                })
            })
            .collect();
        ThreadPool { workers, sender: Some(tx) }
    }

    fn execute<F>(&self, f: F)
    where
        F: FnOnce() + Send + 'static,
    {
        self.sender.as_ref().unwrap().send(Box::new(f)).unwrap();
    }

    fn size(&self) -> usize {
        self.workers.len()
    }
}

impl Drop for ThreadPool {
    fn drop(&mut self) {
        drop(self.sender.take());
        for w in self.workers.drain(..) {
            w.join().unwrap();
        }
    }
}

// Try it with Run.
fn main() {
    let pool = ThreadPool::new(3);
    for i in 0..5 {
        pool.execute(move || println!("job {i} on {:?}", thread::current().id()));
    }
}
