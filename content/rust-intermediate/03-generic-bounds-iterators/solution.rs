use std::fmt::Display;

struct Countdown {
    n: u32,
}

impl Iterator for Countdown {
    type Item = u32;
    fn next(&mut self) -> Option<u32> {
        if self.n == 0 {
            None
        } else {
            let v = self.n;
            self.n -= 1;
            Some(v)
        }
    }
}

fn largest_by_key<T, K, F>(items: &[T], key: F) -> Option<&T>
where
    K: PartialOrd,
    F: Fn(&T) -> K,
{
    let mut best: Option<&T> = None;
    for item in items {
        match best {
            Some(b) if key(b) >= key(item) => {}
            _ => best = Some(item),
        }
    }
    best
}

fn summarize<I>(it: I) -> String
where
    I: IntoIterator,
    I::Item: Display,
{
    it.into_iter().map(|x| x.to_string()).collect::<Vec<_>>().join(", ")
}

fn every_other<I: Iterator>(it: I) -> Vec<I::Item> {
    it.step_by(2).collect()
}

// Try it with Run.
fn main() {
    println!("{}", summarize(Countdown { n: 3 }));
    println!("{:?}", largest_by_key(&["pear", "fig", "banana"], |s| s.len()));
    println!("{:?}", every_other(1..=7));
}
