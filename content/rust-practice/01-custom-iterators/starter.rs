struct Fib {
    a: u64,
    b: u64,
}

impl Iterator for Fib {
    type Item = u64;

    fn next(&mut self) -> Option<u64> {
        None
    }
}

fn fib() -> Fib {
    Fib { a: 0, b: 1 }
}

struct Countdown {
    n: u32,
}

impl Iterator for Countdown {
    type Item = u32;

    fn next(&mut self) -> Option<u32> {
        None
    }
}

fn countdown(from: u32) -> Countdown {
    Countdown { n: from }
}

fn sum_even_fibs_below(limit: u64) -> u64 {
    0
}

// Try it with Run.
fn main() {
    let first: Vec<u64> = fib().take(8).collect();
    println!("{:?}", first);
    println!("{:?}", countdown(3).collect::<Vec<_>>());
    println!("{}", sum_even_fibs_below(100));
}
