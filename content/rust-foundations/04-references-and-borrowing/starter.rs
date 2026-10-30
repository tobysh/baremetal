fn swap(a: &mut i32, b: &mut i32) {
    // your code here
}

fn double_all(v: &mut Vec<i32>) {
    // your code here
}

fn count_matching(words: &Vec<String>, target: &str) -> usize {
    0
}

fn largest_mut(v: &mut Vec<i32>) -> Option<&mut i32> {
    None
}

// Try your functions with Run.
fn main() {
    let (mut a, mut b) = (1, 2);
    swap(&mut a, &mut b);
    println!("{} {}", a, b);

    let mut v = vec![3, 9, 4];
    double_all(&mut v);
    if let Some(m) = largest_mut(&mut v) {
        *m = 0;
    }
    println!("{:?}", v);
}
