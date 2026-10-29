#[test]
fn multiples_are_endless_and_lazy() {
    assert_eq!(multiples_of(3).take(4).collect::<Vec<_>>(), vec![3, 6, 9, 12], "multiples of 3");
    assert_eq!(multiples_of(10).nth(99), Some(1000), "the 100th multiple of 10");
    assert_eq!(multiples_of(0).take(2).collect::<Vec<_>>(), vec![0, 0], "zero");
}

#[test]
fn collatz_sequences() {
    assert_eq!(collatz(6).collect::<Vec<_>>(), vec![6, 3, 10, 5, 16, 8, 4, 2, 1], "from 6");
    assert_eq!(collatz(1).collect::<Vec<_>>(), vec![1], "from 1");
    assert_eq!(collatz(0).count(), 0, "from 0 is empty");
    assert_eq!(collatz(27).count(), 112, "27 takes 111 steps, so 112 values");
    assert_eq!(collatz(27).max(), Some(9232), "27 peaks at 9232");
}

#[test]
fn digits_come_out_backwards() {
    assert_eq!(digits_rev(1250).collect::<Vec<_>>(), vec![0, 5, 2, 1], "1250");
    assert_eq!(digits_rev(7).collect::<Vec<_>>(), vec![7], "one digit");
    assert_eq!(digits_rev(0).collect::<Vec<_>>(), vec![0], "zero is one digit");
    assert_eq!(digits_rev(4_000_000_000).collect::<Vec<_>>(), vec![0, 0, 0, 0, 0, 0, 0, 0, 0, 4], "large");
    assert_eq!(digits_rev(909).sum::<u32>(), 18, "adaptors work on the result");
}

#[test]
fn pick_by_kind() {
    assert_eq!(pick("even", 9).collect::<Vec<_>>(), vec![0, 2, 4, 6, 8], "even");
    assert_eq!(pick("odd", 9).collect::<Vec<_>>(), vec![1, 3, 5, 7], "odd");
    assert_eq!(pick("all", 3).collect::<Vec<_>>(), vec![0, 1, 2], "all");
    assert_eq!(pick("all", 0).count(), 0, "limit 0");
    assert_eq!(pick("nope", 100).count(), 0, "unknown kind");
    assert_eq!(pick("even", 1_000_000).take(3).collect::<Vec<_>>(), vec![0, 2, 4], "lazy for a big limit");
}
