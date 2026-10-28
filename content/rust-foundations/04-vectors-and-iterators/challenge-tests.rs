#[test]
fn empty_has_no_median() {
    assert_eq!(median(&[]), None, "median of an empty slice is None");
}

#[test]
fn odd_count_takes_the_middle() {
    assert_eq!(median(&[5, 1, 3]), Some(3.0), "median of [5, 1, 3] is 3 (sort first)");
    assert_eq!(median(&[7]), Some(7.0), "one value is its own median");
}

#[test]
fn even_count_averages_the_middle_two() {
    assert_eq!(median(&[4, 1, 3, 2]), Some(2.5), "median of [1, 2, 3, 4] is 2.5");
    assert_eq!(median(&[10, 20]), Some(15.0), "median of [10, 20] is 15");
}

#[test]
fn negatives_and_duplicates() {
    assert_eq!(median(&[-5, -1, -1, -9]), Some(-3.0), "sorted: -9 -5 -1 -1, middle two are -5 and -1");
    assert_eq!(median(&[2, 2, 2, 2, 2]), Some(2.0), "all equal");
}

#[test]
fn does_not_overflow_on_large_values() {
    assert_eq!(median(&[i32::MAX, i32::MAX]), Some(i32::MAX as f64), "averaging two huge values must not overflow");
    assert_eq!(median(&[i32::MIN, i32::MAX]), Some(-0.5), "MIN and MAX average to -0.5");
}

#[test]
fn does_not_change_the_input() {
    let data = [9, 3, 7, 1];
    let copy = data;
    let _ = median(&data);
    assert_eq!(data, copy, "the slice you were given must stay in its original order");
}
