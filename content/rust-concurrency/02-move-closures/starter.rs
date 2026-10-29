use std::thread::{self, JoinHandle};

fn greet_all(names: Vec<String>) -> Vec<String> {
    Vec::new()
}

fn indexed_squares(n: usize) -> Vec<usize> {
    Vec::new()
}

fn spawn_range_sum(start: u32, len: u32) -> JoinHandle<u32> {
    thread::spawn(|| 0)
}

// Try it with Run.
fn main() {
    println!("{:?}", greet_all(vec!["Ada".to_string(), "Linus".to_string()]));
    println!("{:?}", indexed_squares(5));
    println!("{:?}", spawn_range_sum(1, 4).join());
}
