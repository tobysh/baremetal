use std::sync::atomic::{AtomicBool, AtomicI64, AtomicU64, AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread::{self, JoinHandle};

fn count_atomic(threads: usize, per: usize) -> usize {
    0
}

static NEXT_ID: AtomicU64 = AtomicU64::new(1);

fn next_id() -> u64 {
    0
}

fn record_max(cur: &AtomicI64, v: i64) {
}

struct Stop {
    flag: AtomicBool,
}

impl Stop {
    fn new() -> Stop {
        Stop { flag: AtomicBool::new(false) }
    }

    fn request(&self) {
    }

    fn is_requested(&self) -> bool {
        false
    }
}

fn spawn_counter(stop: Arc<Stop>) -> JoinHandle<u64> {
    thread::spawn(|| 0)
}

// Try it with Run.
fn main() {
    println!("{}", count_atomic(4, 1000));
    println!("{} {}", next_id(), next_id());
}
