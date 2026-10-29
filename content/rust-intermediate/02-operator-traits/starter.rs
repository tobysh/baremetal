use std::fmt;
use std::iter::Sum;
use std::ops::{Add, AddAssign, Mul, Neg, Sub};

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
struct Cents(i64);

impl Add for Cents {
    type Output = Cents;
    fn add(self, o: Cents) -> Cents {
        Cents(0)
    }
}

impl Sub for Cents {
    type Output = Cents;
    fn sub(self, o: Cents) -> Cents {
        Cents(0)
    }
}

impl Mul<i64> for Cents {
    type Output = Cents;
    fn mul(self, k: i64) -> Cents {
        Cents(0)
    }
}

impl Neg for Cents {
    type Output = Cents;
    fn neg(self) -> Cents {
        Cents(0)
    }
}

impl AddAssign for Cents {
    fn add_assign(&mut self, o: Cents) {}
}

impl Sum for Cents {
    fn sum<I: Iterator<Item = Cents>>(iter: I) -> Cents {
        Cents(0)
    }
}

impl fmt::Display for Cents {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        Ok(())
    }
}

// Try it with Run.
fn main() {
    let prices = vec![Cents(1999), Cents(250), Cents(5)];
    let total: Cents = prices.iter().copied().sum();
    println!("{} and {}", total, -total + Cents(100) * 2);
}
