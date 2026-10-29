fn dot(a: &[i64], b: &[i64]) -> i64 {
    0
}

fn running_max(v: &[i32]) -> Vec<i32> {
    Vec::new()
}

fn pairwise_diffs(v: &[i32]) -> Vec<i32> {
    Vec::new()
}

fn chunk_sums(v: &[i32], n: usize) -> Vec<i32> {
    Vec::new()
}

fn positions(v: &[i32], target: i32) -> Vec<usize> {
    Vec::new()
}

// Try it with Run.
fn main() {
    println!("{}", dot(&[1, 2, 3], &[4, 5, 6]));
    println!("{:?}", running_max(&[3, 1, 4, 1, 5]));
    println!("{:?}", pairwise_diffs(&[1, 4, 9, 16]));
    println!("{:?}", chunk_sums(&[1, 2, 3, 4, 5], 2));
    println!("{:?}", positions(&[1, 2, 1], 1));
}
