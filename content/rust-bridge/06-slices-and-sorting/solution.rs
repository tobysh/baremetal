fn median(v: &[i32]) -> Option<f64> {
    if v.is_empty() {
        return None;
    }
    let mut s = v.to_vec();
    s.sort();
    let n = s.len();
    if n % 2 == 1 {
        Some(s[n / 2] as f64)
    } else {
        Some((s[n / 2 - 1] as i64 + s[n / 2] as i64) as f64 / 2.0)
    }
}

fn top_n(v: &[i32], n: usize) -> Vec<i32> {
    let mut s = v.to_vec();
    s.sort_by(|a, b| b.cmp(a));
    s.truncate(n);
    s
}

fn insert_sorted(v: &mut Vec<i32>, x: i32) {
    let i = match v.binary_search(&x) {
        Ok(i) | Err(i) => i,
    };
    v.insert(i, x);
}

fn unique_sorted(v: &[i32]) -> Vec<i32> {
    let mut s = v.to_vec();
    s.sort();
    s.dedup();
    s
}

fn ends(v: &[i32]) -> Option<(i32, i32)> {
    match v {
        [] => None,
        [x] => Some((*x, *x)),
        [first, .., last] => Some((*first, *last)),
    }
}

fn sort_people(people: &mut Vec<(String, u32)>) {
    people.sort_by(|a, b| b.1.cmp(&a.1).then_with(|| a.0.cmp(&b.0)));
}

fn main() {
    println!("{:?}", median(&[5, 1, 3, 2]));
}
