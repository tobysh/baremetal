use std::thread::{self, JoinHandle};

fn greet_all(names: Vec<String>) -> Vec<String> {
    let handles: Vec<_> = names
        .into_iter()
        .map(|n| thread::spawn(move || format!("hello, {n}")))
        .collect();
    handles.into_iter().map(|h| h.join().unwrap()).collect()
}

fn indexed_squares(n: usize) -> Vec<usize> {
    let handles: Vec<_> = (0..n).map(|i| thread::spawn(move || i * i)).collect();
    handles.into_iter().map(|h| h.join().unwrap()).collect()
}

fn spawn_range_sum(start: u32, len: u32) -> JoinHandle<u32> {
    thread::spawn(move || (start..start + len).sum())
}

// Try it with Run.
fn main() {
    println!("{:?}", greet_all(vec!["Ada".to_string(), "Linus".to_string()]));
    println!("{:?}", indexed_squares(5));
    println!("{:?}", spawn_range_sum(1, 4).join());
}
