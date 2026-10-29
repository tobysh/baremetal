use std::sync::mpsc;
use std::thread;

fn fan_out_sum(chunks: Vec<Vec<u64>>) -> u64 {
    0
}

fn squares_in_order(inputs: Vec<u32>) -> Vec<u64> {
    Vec::new()
}

fn collect_from_producers(producers: usize, per: usize) -> Vec<Vec<usize>> {
    Vec::new()
}

fn pipeline(input: Vec<i32>) -> Vec<i32> {
    Vec::new()
}

// Try it with Run.
fn main() {
    println!("{}", fan_out_sum(vec![vec![1, 2, 3], vec![10, 20]]));
    println!("{:?}", squares_in_order(vec![3, 1, 4]));
    println!("{:?}", pipeline(vec![1, 2, 3]));
}
