use std::collections::HashSet;
use std::sync::{Arc, Mutex};
use std::thread::{self, ThreadId};

fn tally_on_threads(n: usize) -> (usize, usize) {
    let total = Arc::new(Mutex::new(0usize));
    let ids = Arc::new(Mutex::new(HashSet::new()));
    let handles: Vec<_> = (0..n)
        .map(|_| {
            let total = Arc::clone(&total);
            let ids = Arc::clone(&ids);
            thread::spawn(move || {
                *total.lock().unwrap() += 1;
                ids.lock().unwrap().insert(thread::current().id());
            })
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }
    let result = (*total.lock().unwrap(), ids.lock().unwrap().len());
    result
}

fn round_trip<T: Send + 'static>(value: T) -> (T, ThreadId) {
    thread::spawn(move || (value, thread::current().id())).join().unwrap()
}

// Try it with Run.
fn main() {
    println!("{:?}", tally_on_threads(4));
    println!("{:?}", round_trip(String::from("hi")).0);
}
