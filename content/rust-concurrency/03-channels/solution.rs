use std::sync::mpsc::{self, Receiver};
use std::thread;

fn fan_in(producers: usize, per: usize) -> Vec<usize> {
    let (tx, rx) = mpsc::channel();
    for p in 0..producers {
        let tx = tx.clone();
        thread::spawn(move || {
            for k in 0..per {
                tx.send(p * 1000 + k).unwrap();
            }
        });
    }
    drop(tx);
    let mut all: Vec<usize> = rx.iter().collect();
    all.sort();
    all
}

fn pipeline(input: Vec<i32>) -> Vec<i32> {
    let (in_tx, in_rx) = mpsc::channel::<i32>();
    let (out_tx, out_rx) = mpsc::channel::<i32>();
    let worker = thread::spawn(move || {
        for n in in_rx {
            out_tx.send(n * 2 + 1).unwrap();
        }
    });
    for n in input {
        in_tx.send(n).unwrap();
    }
    drop(in_tx);
    let out: Vec<i32> = out_rx.iter().collect();
    worker.join().unwrap();
    out
}

fn drain_ready(rx: &Receiver<i32>) -> Vec<i32> {
    rx.try_iter().collect()
}

// Try it with Run.
fn main() {
    println!("{:?}", fan_in(3, 2));
    println!("{:?}", pipeline(vec![1, 2, 3]));
}
