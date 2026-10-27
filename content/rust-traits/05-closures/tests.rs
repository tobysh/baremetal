#[test]
fn adder_adds() {
    let add5 = make_adder(5);
    assert_eq!(add5(10), 15, "5 + 10");
    assert_eq!(add5(-5), 0, "5 + -5");
}

#[test]
fn adders_are_independent() {
    let a = make_adder(1);
    let b = make_adder(100);
    assert_eq!((a(1), b(1)), (2, 101), "each adder keeps its own n");
}

#[test]
fn apply_n_doubles() {
    assert_eq!(apply_n(|x| x * 2, 1, 5), 32, "doubling 1 five times is 32");
}

#[test]
fn apply_n_zero_times_returns_input() {
    assert_eq!(apply_n(|x| x + 1000, 7, 0), 7, "zero applications leave the value alone");
}

#[test]
fn apply_n_with_captured_value() {
    let step = 3;
    assert_eq!(apply_n(|x| x - step, 20, 4), 8, "20 - 3*4");
}

#[test]
fn apply_n_with_make_adder() {
    assert_eq!(apply_n(make_adder(10), 0, 3), 30, "adding 10 three times");
}

#[test]
fn counter_counts_up() {
    let mut c = make_counter();
    assert_eq!(c(), 1, "first call");
    assert_eq!(c(), 2, "second call");
    assert_eq!(c(), 3, "third call");
}

#[test]
fn counters_are_independent() {
    let mut a = make_counter();
    let mut b = make_counter();
    a();
    a();
    assert_eq!(b(), 1, "b has its own count");
    assert_eq!(a(), 3, "a continues from its own count");
}
