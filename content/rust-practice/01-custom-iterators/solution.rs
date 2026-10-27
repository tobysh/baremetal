struct Fib {
    a: u64,
    b: u64,
}

impl Iterator for Fib {
    type Item = u64;

    fn next(&mut self) -> Option<u64> {
        let next = self.a.checked_add(self.b)?;
        let out = self.a;
        self.a = self.b;
        self.b = next;
        Some(out)
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
        if self.n == 0 {
            return None;
        }
        let out = self.n;
        self.n -= 1;
        Some(out)
    }
}

fn countdown(from: u32) -> Countdown {
    Countdown { n: from }
}

fn sum_even_fibs_below(limit: u64) -> u64 {
    fib().take_while(|&n| n < limit).filter(|n| n % 2 == 0).sum()
}

// Try it with Run.
fn main() {
    let first: Vec<u64> = fib().take(8).collect();
    println!("{:?}", first);
    println!("{:?}", countdown(3).collect::<Vec<_>>());
    println!("{}", sum_even_fibs_below(100));
}
