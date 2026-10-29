use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Barrier, Mutex, Once, OnceLock};
use std::thread;

fn lockstep(n: usize, rounds: usize) -> Vec<Vec<usize>> {
    Vec::new()
}

fn leader_count(n: usize, rounds: usize) -> usize {
    0
}

static INIT: Once = Once::new();
static INIT_RUNS: AtomicUsize = AtomicUsize::new(0);

fn ensure_init() {
    // Wrong: runs every time.
    INIT_RUNS.fetch_add(1, Ordering::SeqCst);
}

static CONFIG: OnceLock<String> = OnceLock::new();
static CONFIG_BUILDS: AtomicUsize = AtomicUsize::new(0);

fn config() -> &'static String {
    // Wrong: builds (and leaks) a fresh string every call.
    CONFIG_BUILDS.fetch_add(1, Ordering::SeqCst);
    Box::leak(Box::new(String::from("config-v1")))
}

// Try it with Run.
fn main() {
    ensure_init();
    ensure_init();
    println!("{} {}", INIT_RUNS.load(Ordering::SeqCst), config());
}
