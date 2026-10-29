use std::collections::BTreeMap;
use std::thread;

fn map_reduce<T, U, M, R>(items: &[T], workers: usize, map: M, identity: U, reduce: R) -> U
where
    T: Sync,
    U: Send + Clone,
    M: Fn(&T) -> U + Sync,
    R: Fn(U, U) -> U + Sync,
{
    identity
}

fn word_frequencies(texts: &[&str], workers: usize) -> BTreeMap<String, usize> {
    BTreeMap::new()
}

// Try it with Run.
fn main() {
    let nums: Vec<u64> = (1..=10).collect();
    println!("{}", map_reduce(&nums, 3, |n| n * n, 0, |a, b| a + b));
    println!("{:?}", word_frequencies(&["a b a", "B"], 2));
}
