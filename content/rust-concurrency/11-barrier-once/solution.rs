use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Barrier, Mutex, Once, OnceLock};
use std::thread;

fn lockstep(n: usize, rounds: usize) -> Vec<Vec<usize>> {
    let barrier = Arc::new(Barrier::new(n));
    let counter = Arc::new(Mutex::new(0usize));
    let handles: Vec<_> = (0..n)
        .map(|_| {
            let barrier = Arc::clone(&barrier);
            let counter = Arc::clone(&counter);
            thread::spawn(move || {
                let mut seen = Vec::new();
                for _ in 0..rounds {
                    *counter.lock().unwrap() += 1;
                    barrier.wait();
                    seen.push(*counter.lock().unwrap());
                    barrier.wait();
                }
                seen
            })
        })
        .collect();
    handles.into_iter().map(|h| h.join().unwrap()).collect()
}

fn leader_count(n: usize, rounds: usize) -> usize {
    let barrier = Arc::new(Barrier::new(n));
    let handles: Vec<_> = (0..n)
        .map(|_| {
            let barrier = Arc::clone(&barrier);
            thread::spawn(move || {
                let mut leaders = 0;
                for _ in 0..rounds {
                    if barrier.wait().is_leader() {
                        leaders += 1;
                    }
                }
                leaders
            })
        })
        .collect();
    handles.into_iter().map(|h| h.join().unwrap()).sum()
}

static INIT: Once = Once::new();
static INIT_RUNS: AtomicUsize = AtomicUsize::new(0);

fn ensure_init() {
    INIT.call_once(|| {
        INIT_RUNS.fetch_add(1, Ordering::SeqCst);
    });
}

static CONFIG: OnceLock<String> = OnceLock::new();
static CONFIG_BUILDS: AtomicUsize = AtomicUsize::new(0);

fn config() -> &'static String {
    CONFIG.get_or_init(|| {
        CONFIG_BUILDS.fetch_add(1, Ordering::SeqCst);
        String::from("config-v1")
    })
}

// Try it with Run.
fn main() {
    ensure_init();
    ensure_init();
    println!("{} {}", INIT_RUNS.load(Ordering::SeqCst), config());
}
