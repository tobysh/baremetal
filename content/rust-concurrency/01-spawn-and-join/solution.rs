use std::thread::{self, ThreadId};

fn sum_chunks(chunks: Vec<Vec<u64>>) -> Vec<u64> {
    let handles: Vec<_> = chunks
        .into_iter()
        .map(|c| thread::spawn(move || c.iter().sum::<u64>()))
        .collect();
    handles.into_iter().map(|h| h.join().unwrap()).collect()
}

fn run_guarded(f: fn() -> i32) -> Result<i32, String> {
    match thread::spawn(f).join() {
        Ok(v) => Ok(v),
        Err(_) => Err(String::from("worker panicked")),
    }
}

fn on_new_thread(f: fn() -> u64) -> (u64, ThreadId) {
    thread::spawn(move || (f(), thread::current().id())).join().unwrap()
}

// Try it with Run.
fn main() {
    println!("{:?}", sum_chunks(vec![vec![1, 2, 3], vec![10, 20]]));
    println!("{:?}", run_guarded(|| 7));
}
