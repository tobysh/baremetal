#[test]
fn fib_starts_correctly() {
    let v: Vec<u64> = fib().take(10).collect();
    assert_eq!(v, vec![0, 1, 1, 2, 3, 5, 8, 13, 21, 34], "the first ten Fibonacci numbers");
}

#[test]
fn fib_works_with_adaptors() {
    let v: Vec<u64> = fib().skip(5).step_by(2).take(3).collect();
    assert_eq!(v, vec![5, 13, 34], "skip(5).step_by(2).take(3) should give 5, 13, 34");
}

#[test]
fn fib_ends_instead_of_overflowing() {
    let n = fib().count();
    assert!(n > 80 && n < 100, "fib should be finite and yield roughly 90 numbers, got {}", n);
}

#[test]
fn countdown_counts_down() {
    let v: Vec<u32> = countdown(4).collect();
    assert_eq!(v, vec![4, 3, 2, 1], "countdown(4) should give 4, 3, 2, 1");
}

#[test]
fn countdown_of_zero_is_empty() {
    assert_eq!(countdown(0).count(), 0, "countdown(0) should yield nothing");
}

#[test]
fn countdown_stays_finished() {
    let mut c = countdown(1);
    assert_eq!(c.next(), Some(1), "first call gives 1");
    assert_eq!(c.next(), None, "then None");
    assert_eq!(c.next(), None, "and still None");
}

#[test]
fn countdown_sum() {
    let total: u32 = countdown(10).sum();
    assert_eq!(total, 55, "10 + 9 + ... + 1 is 55");
}

#[test]
fn even_fibs_below_100() {
    assert_eq!(sum_even_fibs_below(100), 44, "0 + 2 + 8 + 34 is 44");
}

#[test]
fn even_fibs_limit_is_exclusive() {
    assert_eq!(sum_even_fibs_below(34), 10, "34 is not below 34, so only 0 + 2 + 8");
}

#[test]
fn even_fibs_below_zero_and_small() {
    assert_eq!(sum_even_fibs_below(0), 0, "nothing is below 0");
    assert_eq!(sum_even_fibs_below(2), 0, "only 0 is even and below 2");
}

#[test]
fn even_fibs_below_four_million() {
    assert_eq!(sum_even_fibs_below(4_000_000), 4_613_732, "the classic answer");
}
