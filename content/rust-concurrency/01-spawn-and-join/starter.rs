use std::thread::{self, ThreadId};

fn sum_chunks(chunks: Vec<Vec<u64>>) -> Vec<u64> {
    // Spawn one thread per chunk, then join them in order.
    // Hint: `thread::spawn(move || ...)` returns a JoinHandle.
    Vec::new()
}

fn run_guarded(f: fn() -> i32) -> Result<i32, String> {
    Ok(0)
}

fn on_new_thread(f: fn() -> u64) -> (u64, ThreadId) {
    // Placeholder: this runs on the *current* thread, which is the bug.
    (f(), thread::current().id())
}

// Try it with Run.
fn main() {
    println!("{:?}", sum_chunks(vec![vec![1, 2, 3], vec![10, 20]]));
    println!("{:?}", run_guarded(|| 7));
}
