use std::thread;

fn chunk_size(len: usize, parts: usize) -> usize {
    let parts = parts.max(1);
    ((len + parts - 1) / parts).max(1)
}

fn parallel_sum(data: &[u64], parts: usize) -> u64 {
    let size = chunk_size(data.len(), parts);
    thread::scope(|s| {
        let handles: Vec<_> = data
            .chunks(size)
            .map(|c| s.spawn(move || c.iter().sum::<u64>()))
            .collect();
        handles.into_iter().map(|h| h.join().unwrap()).sum()
    })
}

fn increment_all(data: &mut [i32], parts: usize) {
    let size = chunk_size(data.len(), parts);
    thread::scope(|s| {
        for chunk in data.chunks_mut(size) {
            s.spawn(move || {
                for x in chunk.iter_mut() {
                    *x += 1;
                }
            });
        }
    });
}

fn min_max(data: &[i32]) -> Option<(i32, i32)> {
    if data.is_empty() {
        return None;
    }
    Some(thread::scope(|s| {
        let lo = s.spawn(|| *data.iter().min().unwrap());
        let hi = s.spawn(|| *data.iter().max().unwrap());
        (lo.join().unwrap(), hi.join().unwrap())
    }))
}

// Try it with Run.
fn main() {
    let v: Vec<u64> = (1..=100).collect();
    println!("{}", parallel_sum(&v, 4));
    let mut w = vec![1, 2, 3];
    increment_all(&mut w, 2);
    println!("{:?} {:?}", w, min_max(&w));
}
