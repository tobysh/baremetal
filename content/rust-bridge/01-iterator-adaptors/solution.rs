fn dot(a: &[i64], b: &[i64]) -> i64 {
    a.iter().zip(b).map(|(x, y)| x * y).sum()
}

fn running_max(v: &[i32]) -> Vec<i32> {
    v.iter()
        .scan(i32::MIN, |m, &x| {
            *m = (*m).max(x);
            Some(*m)
        })
        .collect()
}

fn pairwise_diffs(v: &[i32]) -> Vec<i32> {
    v.windows(2).map(|w| w[1] - w[0]).collect()
}

fn chunk_sums(v: &[i32], n: usize) -> Vec<i32> {
    if n == 0 {
        return Vec::new();
    }
    v.chunks(n).map(|c| c.iter().sum()).collect()
}

fn positions(v: &[i32], target: i32) -> Vec<usize> {
    v.iter()
        .enumerate()
        .filter(|(_, &x)| x == target)
        .map(|(i, _)| i)
        .collect()
}

fn main() {
    println!("{}", dot(&[1, 2, 3], &[4, 5, 6]));
}
