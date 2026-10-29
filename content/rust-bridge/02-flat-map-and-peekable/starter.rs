fn run_lengths(s: &str) -> Vec<(char, usize)> {
    Vec::new()
}

fn all_words(lines: &[&str]) -> Vec<String> {
    Vec::new()
}

fn split_by_sign(v: &[i32]) -> (Vec<i32>, Vec<i32>) {
    (Vec::new(), Vec::new())
}

fn first_big_square(v: &[i32], limit: i32) -> Option<i32> {
    None
}

fn numbers_in(items: &[&str]) -> Vec<i32> {
    Vec::new()
}

// Try it with Run.
fn main() {
    println!("{:?}", run_lengths("aaabcc"));
    println!("{:?}", all_words(&["Hello World", "again"]));
    println!("{:?}", split_by_sign(&[3, -1, 0, -7, 2]));
    println!("{:?}", first_big_square(&[1, 2, 5, 9], 20));
    println!("{:?}", numbers_in(&["12", "x1", "-4"]));
}
