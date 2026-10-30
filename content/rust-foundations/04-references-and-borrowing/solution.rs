fn swap(a: &mut i32, b: &mut i32) {
    let t = *a;
    *a = *b;
    *b = t;
}

fn double_all(v: &mut Vec<i32>) {
    for x in v.iter_mut() {
        *x *= 2;
    }
}

fn count_matching(words: &Vec<String>, target: &str) -> usize {
    words.iter().filter(|w| w.as_str() == target).count()
}

fn largest_mut(v: &mut Vec<i32>) -> Option<&mut i32> {
    v.iter_mut().max_by_key(|x| **x)
}

fn main() {
    let (mut a, mut b) = (1, 2);
    swap(&mut a, &mut b);
    println!("{} {}", a, b);
}
