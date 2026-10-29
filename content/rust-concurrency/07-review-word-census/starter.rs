use std::collections::{BTreeMap, HashMap};
use std::sync::{mpsc, Arc, Mutex};
use std::thread;

fn census(lines: Vec<String>, workers: usize) -> (BTreeMap<String, usize>, usize) {
    (BTreeMap::new(), 0)
}

// Try it with Run.
fn main() {
    let lines = vec!["the cat".to_string(), "The dog and the cat".to_string()];
    let (counts, total) = census(lines, 2);
    println!("{:?} {}", counts, total);
}
