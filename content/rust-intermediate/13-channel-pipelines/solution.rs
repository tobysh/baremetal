use std::sync::mpsc;
use std::thread;

fn fan_out_sum(chunks: Vec<Vec<u64>>) -> u64 {
    let (tx, rx) = mpsc::channel();
    for chunk in chunks {
        let tx = tx.clone();
        thread::spawn(move || {
            tx.send(chunk.iter().sum::<u64>()).unwrap();
        });
    }
    drop(tx);
    rx.iter().sum()
}

fn squares_in_order(inputs: Vec<u32>) -> Vec<u64> {
    let (tx, rx) = mpsc::channel();
    for (i, v) in inputs.into_iter().enumerate() {
        let tx = tx.clone();
        thread::spawn(move || {
            tx.send((i, v as u64 * v as u64)).unwrap();
        });
    }
    drop(tx);
    let mut got: Vec<(usize, u64)> = rx.iter().collect();
    got.sort();
    got.into_iter().map(|(_, v)| v).collect()
}

fn collect_from_producers(producers: usize, per: usize) -> Vec<Vec<usize>> {
    let (tx, rx) = mpsc::channel();
    for p in 0..producers {
        let tx = tx.clone();
        thread::spawn(move || {
            for k in 0..per {
                tx.send((p, k)).unwrap();
            }
        });
    }
    drop(tx);
    let mut result = vec![Vec::new(); producers];
    for (p, k) in rx {
        result[p].push(k);
    }
    result
}

fn pipeline(input: Vec<i32>) -> Vec<i32> {
    let (tx1, rx1) = mpsc::channel();
    let (tx2, rx2) = mpsc::channel();
    let (tx3, rx3) = mpsc::channel();
    let a = thread::spawn(move || {
        for x in rx1 {
            tx2.send(x * 2).unwrap();
        }
    });
    let b = thread::spawn(move || {
        for x in rx2 {
            tx3.send(x + 1).unwrap();
        }
    });
    for x in input {
        tx1.send(x).unwrap();
    }
    drop(tx1);
    let out: Vec<i32> = rx3.iter().collect();
    a.join().unwrap();
    b.join().unwrap();
    out
}

// Try it with Run.
fn main() {
    println!("{}", fan_out_sum(vec![vec![1, 2, 3], vec![10, 20]]));
    println!("{:?}", squares_in_order(vec![3, 1, 4]));
    println!("{:?}", pipeline(vec![1, 2, 3]));
}
