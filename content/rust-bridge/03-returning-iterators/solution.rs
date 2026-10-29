use std::iter::successors;

fn multiples_of(k: u32) -> impl Iterator<Item = u32> {
    (1..).map(move |i| i * k)
}

fn collatz(start: u64) -> impl Iterator<Item = u64> {
    let first = if start == 0 { None } else { Some(start) };
    successors(first, |&n| {
        if n == 1 {
            None
        } else if n % 2 == 0 {
            Some(n / 2)
        } else {
            Some(3 * n + 1)
        }
    })
}

fn digits_rev(n: u32) -> impl Iterator<Item = u32> {
    successors(Some(n), |&x| if x >= 10 { Some(x / 10) } else { None }).map(|x| x % 10)
}

fn pick(kind: &str, limit: u32) -> Box<dyn Iterator<Item = u32>> {
    match kind {
        "even" => Box::new((0..limit).filter(|n| n % 2 == 0)),
        "odd" => Box::new((0..limit).filter(|n| n % 2 == 1)),
        "all" => Box::new(0..limit),
        _ => Box::new(std::iter::empty()),
    }
}

fn main() {
    println!("{:?}", collatz(6).collect::<Vec<_>>());
}
