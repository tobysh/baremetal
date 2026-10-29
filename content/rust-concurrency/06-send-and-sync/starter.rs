use std::cell::RefCell;
use std::collections::HashSet;
use std::rc::Rc;
use std::sync::{Arc, Mutex};
use std::thread::{self, ThreadId};

// Works, but on one thread only. Rc and RefCell are neither Send nor Sync,
// so thread::spawn would reject them. Rewrite with Arc and Mutex.
fn tally_on_threads(n: usize) -> (usize, usize) {
    let total = Rc::new(RefCell::new(0usize));
    let ids = Rc::new(RefCell::new(HashSet::new()));
    for _ in 0..n {
        *total.borrow_mut() += 1;
        ids.borrow_mut().insert(thread::current().id());
    }
    let result = (*total.borrow(), ids.borrow().len());
    result
}

fn round_trip<T: Send + 'static>(value: T) -> (T, ThreadId) {
    (value, thread::current().id())
}

// Try it with Run.
fn main() {
    println!("{:?}", tally_on_threads(4));
    println!("{:?}", round_trip(String::from("hi")).0);
}
