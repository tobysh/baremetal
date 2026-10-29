use std::sync::atomic::{AtomicBool, AtomicI64, AtomicU64, AtomicUsize, Ordering};
use std::sync::Arc;
use std::thread::{self, JoinHandle};

fn count_atomic(threads: usize, per: usize) -> usize {
    let c = Arc::new(AtomicUsize::new(0));
    let handles: Vec<_> = (0..threads)
        .map(|_| {
            let c = Arc::clone(&c);
            thread::spawn(move || {
                for _ in 0..per {
                    c.fetch_add(1, Ordering::Relaxed);
                }
            })
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }
    c.load(Ordering::SeqCst)
}

static NEXT_ID: AtomicU64 = AtomicU64::new(1);

fn next_id() -> u64 {
    NEXT_ID.fetch_add(1, Ordering::Relaxed)
}

fn record_max(cur: &AtomicI64, v: i64) {
    cur.fetch_max(v, Ordering::Relaxed);
}

struct Stop {
    flag: AtomicBool,
}

impl Stop {
    fn new() -> Stop {
        Stop { flag: AtomicBool::new(false) }
    }

    fn request(&self) {
        self.flag.store(true, Ordering::Release);
    }

    fn is_requested(&self) -> bool {
        self.flag.load(Ordering::Acquire)
    }
}

fn spawn_counter(stop: Arc<Stop>) -> JoinHandle<u64> {
    thread::spawn(move || {
        let mut rounds = 0;
        loop {
            rounds += 1;
            if stop.is_requested() {
                break;
            }
            thread::yield_now();
        }
        rounds
    })
}

// Try it with Run.
fn main() {
    println!("{}", count_atomic(4, 1000));
    println!("{} {}", next_id(), next_id());
}
