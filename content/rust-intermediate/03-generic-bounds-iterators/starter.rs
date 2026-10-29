use std::fmt::Display;

struct Countdown {
    n: u32,
}

impl Iterator for Countdown {
    type Item = u32;
    fn next(&mut self) -> Option<u32> {
        None
    }
}

fn largest_by_key<T, K, F>(items: &[T], key: F) -> Option<&T>
where
    K: PartialOrd,
    F: Fn(&T) -> K,
{
    None
}

fn summarize<I>(it: I) -> String
where
    I: IntoIterator,
    I::Item: Display,
{
    String::new()
}

fn every_other<I: Iterator>(it: I) -> Vec<I::Item> {
    Vec::new()
}

// Try it with Run.
fn main() {
    println!("{}", summarize(Countdown { n: 3 }));
    println!("{:?}", largest_by_key(&["pear", "fig", "banana"], |s| s.len()));
    println!("{:?}", every_other(1..=7));
}
