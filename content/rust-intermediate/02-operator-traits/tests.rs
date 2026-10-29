#[test]
fn add_and_sub() {
    assert_eq!(Cents(150) + Cents(275), Cents(425), "150 + 275");
    assert_eq!(Cents(10) + Cents(-30), Cents(-20), "adding a negative");
    assert_eq!(Cents(500) - Cents(125), Cents(375), "500 - 125");
    assert_eq!(Cents(0) - Cents(7), Cents(-7), "0 - 7");
}

#[test]
fn mul_and_neg() {
    assert_eq!(Cents(250) * 3, Cents(750), "250 * 3");
    assert_eq!(Cents(99) * 0, Cents(0), "times zero");
    assert_eq!(Cents(40) * -2, Cents(-80), "times a negative");
    assert_eq!(-Cents(5), Cents(-5), "negate");
    assert_eq!(-Cents(-900), Cents(900), "negate a negative");
}

#[test]
fn add_assign() {
    let mut a = Cents(100);
    a += Cents(25);
    a += Cents(-5);
    assert_eq!(a, Cents(120), "+= twice");
}

#[test]
fn sums() {
    let v = vec![Cents(100), Cents(250), Cents(-40)];
    let s: Cents = v.iter().copied().sum();
    assert_eq!(s, Cents(310), "sum of three");
    let none: Cents = Vec::<Cents>::new().into_iter().sum();
    assert_eq!(none, Cents(0), "empty sum");
    let lots = (1..=100).map(|n| Cents(n)).sum::<Cents>();
    assert_eq!(lots, Cents(5050), "sum of 1..=100");
}

#[test]
fn display_positive() {
    assert_eq!(Cents(1234).to_string(), "$12.34", "12.34");
    assert_eq!(Cents(5).to_string(), "$0.05", "padding the cents");
    assert_eq!(Cents(0).to_string(), "$0.00", "zero");
    assert_eq!(Cents(100000).to_string(), "$1000.00", "a thousand");
    assert_eq!(format!("total: {}", Cents(70)), "total: $0.70", "inside a larger string");
}

#[test]
fn display_negative() {
    assert_eq!(Cents(-5).to_string(), "-$0.05", "negative cents");
    assert_eq!(Cents(-1250).to_string(), "-$12.50", "negative dollars");
}

#[test]
fn ordering_still_works() {
    assert!(Cents(5) < Cents(6), "derived ordering");
    assert_eq!(Cents(3).max(Cents(9)), Cents(9), "max");
}
