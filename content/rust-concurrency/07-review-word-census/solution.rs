use std::collections::{BTreeMap, HashMap};
use std::sync::{mpsc, Arc, Mutex};
use std::thread;

fn census(lines: Vec<String>, workers: usize) -> (BTreeMap<String, usize>, usize) {
    let workers = workers.max(1);
    let mut buckets: Vec<Vec<String>> = vec![Vec::new(); workers];
    for (i, line) in lines.into_iter().enumerate() {
        buckets[i % workers].push(line);
    }

    let total = Arc::new(Mutex::new(0usize));
    let (tx, rx) = mpsc::channel::<HashMap<String, usize>>();
    let mut handles = Vec::new();
    for bucket in buckets {
        let tx = tx.clone();
        let total = Arc::clone(&total);
        handles.push(thread::spawn(move || {
            let mut local: HashMap<String, usize> = HashMap::new();
            let mut words = 0;
            for line in &bucket {
                for w in line.split_whitespace() {
                    *local.entry(w.to_lowercase()).or_insert(0) += 1;
                    words += 1;
                }
            }
            *total.lock().unwrap() += words;
            tx.send(local).unwrap();
        }));
    }
    drop(tx);

    let mut merged = BTreeMap::new();
    for map in rx {
        for (w, c) in map {
            *merged.entry(w).or_insert(0) += c;
        }
    }
    for h in handles {
        h.join().unwrap();
    }
    let t = *total.lock().unwrap();
    (merged, t)
}

// Try it with Run.
fn main() {
    let lines = vec!["the cat".to_string(), "The dog and the cat".to_string()];
    let (counts, total) = census(lines, 2);
    println!("{:?} {}", counts, total);
}
