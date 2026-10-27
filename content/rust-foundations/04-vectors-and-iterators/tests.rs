#[test]
fn squares_the_even_numbers() {
    assert_eq!(evens_squared(&[1, 2, 3, 4]), vec![4, 16], "evens_squared(&[1, 2, 3, 4]) should be [4, 16]");
}

#[test]
fn empty_input_gives_empty_output() {
    assert_eq!(evens_squared(&[]), Vec::<i32>::new(), "evens_squared(&[]) should be empty");
}

#[test]
fn all_odd_numbers_give_empty_output() {
    assert_eq!(evens_squared(&[1, 3, 5]), Vec::<i32>::new(), "evens_squared(&[1, 3, 5]) should be empty");
}

#[test]
fn handles_negative_evens_and_zero() {
    assert_eq!(evens_squared(&[-2, 0, -3, 6]), vec![4, 0, 36], "evens_squared(&[-2, 0, -3, 6]) should be [4, 0, 36]");
}

#[test]
fn keeps_original_order() {
    assert_eq!(evens_squared(&[10, 2, 8]), vec![100, 4, 64], "evens_squared(&[10, 2, 8]) should be [100, 4, 64]");
}

#[test]
fn average_of_several_numbers() {
    let r = average(&[1.0, 2.0, 6.0]).expect("average(&[1.0, 2.0, 6.0]) should be Some(3.0), got None");
    assert!((r - 3.0).abs() < 1e-9, "average(&[1.0, 2.0, 6.0]) should be 3.0, got {}", r);
}

#[test]
fn average_of_one_number_is_itself() {
    let r = average(&[4.5]).expect("average(&[4.5]) should be Some(4.5), got None");
    assert!((r - 4.5).abs() < 1e-9, "average(&[4.5]) should be 4.5, got {}", r);
}

#[test]
fn average_of_empty_slice_is_none() {
    assert_eq!(average(&[]), None, "average(&[]) should be None");
}

#[test]
fn average_handles_negative_numbers() {
    let r = average(&[-1.0, 1.0, -4.0, 4.0]).expect("should be Some(0.0), got None");
    assert!(r.abs() < 1e-9, "average(&[-1.0, 1.0, -4.0, 4.0]) should be 0.0, got {}", r);
}
