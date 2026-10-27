#[test]
fn largest_of_integers() {
    assert_eq!(largest(&[3, 9, 4]), Some(9), "largest of [3, 9, 4] should be Some(9)");
}

#[test]
fn largest_with_negatives() {
    assert_eq!(largest(&[-5, -2, -9]), Some(-2), "largest of negatives should be Some(-2)");
}

#[test]
fn largest_of_floats() {
    assert_eq!(largest(&[1.5, 7.25, 0.2]), Some(7.25), "largest of floats should be Some(7.25)");
}

#[test]
fn largest_of_chars() {
    assert_eq!(largest(&['x', 'b', 'q']), Some('x'), "largest of chars should be Some('x')");
}

#[test]
fn largest_of_empty_is_none() {
    let empty: [i32; 0] = [];
    assert_eq!(largest(&empty), None, "an empty slice has no largest item");
}

#[test]
fn largest_of_single_item() {
    assert_eq!(largest(&[42]), Some(42), "a single item is the largest");
}

#[test]
fn swap_exchanges_fields() {
    let p = Pair::new(1, 2).swap();
    assert_eq!(p.first, 2, "first should now be 2");
    assert_eq!(p.second, 1, "second should now be 1");
}

#[test]
fn swap_works_for_strings() {
    let p = Pair::new(String::from("left"), String::from("right")).swap();
    assert_eq!(p.first, "right", "first should now be right");
    assert_eq!(p.second, "left", "second should now be left");
}
