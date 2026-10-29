use std::cmp::Reverse;
use std::collections::{BinaryHeap, VecDeque};

fn josephus(n: usize, k: usize) -> Vec<usize> {
    let mut out = Vec::new();
    if n == 0 || k == 0 {
        return out;
    }
    let mut q: VecDeque<usize> = (1..=n).collect();
    while !q.is_empty() {
        let r = (k - 1) % q.len();
        q.rotate_left(r);
        out.push(q.pop_front().unwrap());
    }
    out
}

fn k_smallest(v: &[i32], k: usize) -> Vec<i32> {
    let mut h = BinaryHeap::new();
    for &x in v {
        h.push(x);
        if h.len() > k {
            h.pop();
        }
    }
    h.into_sorted_vec()
}

fn run_tasks(tasks: &[(u32, &str)]) -> Vec<String> {
    let mut h: BinaryHeap<(u32, String)> = tasks.iter().map(|&(p, n)| (p, n.to_string())).collect();
    let mut out = Vec::new();
    while let Some((_, name)) = h.pop() {
        out.push(name);
    }
    out
}

fn window_maxima(v: &[i32], w: usize) -> Vec<i32> {
    let mut out = Vec::new();
    if w == 0 || w > v.len() {
        return out;
    }
    let mut dq: VecDeque<usize> = VecDeque::new();
    for i in 0..v.len() {
        while let Some(&b) = dq.back() {
            if v[b] <= v[i] {
                dq.pop_back();
            } else {
                break;
            }
        }
        dq.push_back(i);
        if dq[0] + w <= i {
            dq.pop_front();
        }
        if i + 1 >= w {
            out.push(v[dq[0]]);
        }
    }
    let _ = Reverse(0);
    out
}

fn main() {
    println!("{:?}", josephus(7, 3));
}
