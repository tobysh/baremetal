use std::cmp::Reverse;
use std::collections::{BinaryHeap, VecDeque};

fn josephus(n: usize, k: usize) -> Vec<usize> {
    Vec::new()
}

fn k_smallest(v: &[i32], k: usize) -> Vec<i32> {
    Vec::new()
}

fn run_tasks(tasks: &[(u32, &str)]) -> Vec<String> {
    Vec::new()
}

fn window_maxima(v: &[i32], w: usize) -> Vec<i32> {
    Vec::new()
}

// Try it with Run.
fn main() {
    println!("{:?}", josephus(7, 3));
    println!("{:?}", k_smallest(&[9, 4, 7, 1, 8], 2));
    println!("{:?}", run_tasks(&[(1, "sweep"), (5, "deploy"), (5, "backup")]));
    println!("{:?}", window_maxima(&[1, 3, 2, 5, 4], 3));
    let _: Option<Reverse<i32>> = None;
    let _: Option<VecDeque<i32>> = None;
    let _: Option<BinaryHeap<i32>> = None;
}
