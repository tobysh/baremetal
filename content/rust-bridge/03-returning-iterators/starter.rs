fn multiples_of(k: u32) -> impl Iterator<Item = u32> {
    std::iter::empty()
}

fn collatz(start: u64) -> impl Iterator<Item = u64> {
    std::iter::empty()
}

fn digits_rev(n: u32) -> impl Iterator<Item = u32> {
    std::iter::empty()
}

fn pick(kind: &str, limit: u32) -> Box<dyn Iterator<Item = u32>> {
    Box::new(std::iter::empty())
}

// Try it with Run.
fn main() {
    println!("{:?}", multiples_of(3).take(4).collect::<Vec<_>>());
    println!("{:?}", collatz(6).collect::<Vec<_>>());
    println!("{:?}", digits_rev(1250).collect::<Vec<_>>());
    println!("{:?}", pick("odd", 10).collect::<Vec<_>>());
}
