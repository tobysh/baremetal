use std::thread;

fn parallel_sum(data: &[u64], parts: usize) -> u64 {
    0
}

fn increment_all(data: &mut [i32], parts: usize) {
}

fn min_max(data: &[i32]) -> Option<(i32, i32)> {
    None
}

// Try it with Run.
fn main() {
    let v: Vec<u64> = (1..=100).collect();
    println!("{}", parallel_sum(&v, 4));
    let mut w = vec![1, 2, 3];
    increment_all(&mut w, 2);
    println!("{:?} {:?}", w, min_max(&w));
}
