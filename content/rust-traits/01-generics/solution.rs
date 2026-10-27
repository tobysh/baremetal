struct Pair<T> {
    first: T,
    second: T,
}

impl<T> Pair<T> {
    fn new(first: T, second: T) -> Pair<T> {
        Pair { first, second }
    }

    fn swap(self) -> Pair<T> {
        Pair { first: self.second, second: self.first }
    }
}

fn largest<T: PartialOrd + Copy>(items: &[T]) -> Option<T> {
    let mut best = *items.first()?;
    for item in items {
        if *item > best {
            best = *item;
        }
    }
    Some(best)
}

// Try it with Run.
fn main() {
    println!("{:?}", largest(&[3, 9, 4]));
    println!("{:?}", largest(&[1.5, 0.2]));
    let p = Pair::new("a", "b").swap();
    println!("{} {}", p.first, p.second);
}
