use std::fmt;
use std::iter::Sum;
use std::ops::{Add, AddAssign, Mul, Neg, Sub};

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
struct Cents(i64);

impl Add for Cents {
    type Output = Cents;
    fn add(self, o: Cents) -> Cents {
        Cents(self.0 + o.0)
    }
}

impl Sub for Cents {
    type Output = Cents;
    fn sub(self, o: Cents) -> Cents {
        Cents(self.0 - o.0)
    }
}

impl Mul<i64> for Cents {
    type Output = Cents;
    fn mul(self, k: i64) -> Cents {
        Cents(self.0 * k)
    }
}

impl Neg for Cents {
    type Output = Cents;
    fn neg(self) -> Cents {
        Cents(-self.0)
    }
}

impl AddAssign for Cents {
    fn add_assign(&mut self, o: Cents) {
        self.0 += o.0;
    }
}

impl Sum for Cents {
    fn sum<I: Iterator<Item = Cents>>(iter: I) -> Cents {
        iter.fold(Cents(0), |a, b| a + b)
    }
}

impl fmt::Display for Cents {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let sign = if self.0 < 0 { "-" } else { "" };
        let a = self.0.abs();
        write!(f, "{sign}${}.{:02}", a / 100, a % 100)
    }
}

// Try it with Run.
fn main() {
    let prices = vec![Cents(1999), Cents(250), Cents(5)];
    let total: Cents = prices.iter().copied().sum();
    println!("{} and {}", total, -total + Cents(100) * 2);
}
