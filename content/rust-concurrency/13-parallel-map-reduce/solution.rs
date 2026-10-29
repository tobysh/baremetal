use std::collections::BTreeMap;
use std::thread;

fn map_reduce<T, U, M, R>(items: &[T], workers: usize, map: M, identity: U, reduce: R) -> U
where
    T: Sync,
    U: Send + Clone,
    M: Fn(&T) -> U + Sync,
    R: Fn(U, U) -> U + Sync,
{
    let workers = workers.max(1);
    let size = ((items.len() + workers - 1) / workers).max(1);
    let (map, reduce, identity_ref) = (&map, &reduce, &identity);
    let partials: Vec<U> = thread::scope(|s| {
        let handles: Vec<_> = items
            .chunks(size)
            .map(|chunk| {
                s.spawn(move || {
                    chunk
                        .iter()
                        .fold(identity_ref.clone(), |acc, x| reduce(acc, map(x)))
                })
            })
            .collect();
        handles.into_iter().map(|h| h.join().unwrap()).collect()
    });
    partials.into_iter().fold(identity, |acc, p| reduce(acc, p))
}

fn word_frequencies(texts: &[&str], workers: usize) -> BTreeMap<String, usize> {
    map_reduce(
        texts,
        workers,
        |text| {
            let mut m = BTreeMap::new();
            for w in text.split_whitespace() {
                *m.entry(w.to_lowercase()).or_insert(0) += 1;
            }
            m
        },
        BTreeMap::new(),
        |mut a, b| {
            for (w, c) in b {
                *a.entry(w).or_insert(0) += c;
            }
            a
        },
    )
}

// Try it with Run.
fn main() {
    let nums: Vec<u64> = (1..=10).collect();
    println!("{}", map_reduce(&nums, 3, |n| n * n, 0, |a, b| a + b));
    println!("{:?}", word_frequencies(&["a b a", "B"], 2));
}
