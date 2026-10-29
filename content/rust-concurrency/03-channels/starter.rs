use std::sync::mpsc::{self, Receiver};
use std::thread;

fn fan_in(producers: usize, per: usize) -> Vec<usize> {
    Vec::new()
}

fn pipeline(input: Vec<i32>) -> Vec<i32> {
    input
}

fn drain_ready(rx: &Receiver<i32>) -> Vec<i32> {
    Vec::new()
}

// Try it with Run.
fn main() {
    println!("{:?}", fan_in(3, 2));
    println!("{:?}", pipeline(vec![1, 2, 3]));
}
