struct Pair<T> {
    first: T,
    second: T,
}

impl<T> Pair<T> {
    fn new(first: T, second: T) -> Pair<T> {
        Pair { first, second }
    }

    fn swap(self) -> Pair<T> {
        self
    }
}

fn largest<T: PartialOrd + Copy>(items: &[T]) -> Option<T> {
    None
}

// Try it with Run.
fn main() {
    println!("{:?}", largest(&[3, 9, 4]));
    println!("{:?}", largest(&[1.5, 0.2]));
    let p = Pair::new("a", "b").swap();
    println!("{} {}", p.first, p.second);
}
