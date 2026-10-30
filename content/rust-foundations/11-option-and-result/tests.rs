#[test]
fn find_index_locates_first_match() {
    assert_eq!(find_index(&[5, 6, 7], 6), Some(1), "6 is at index 1");
    assert_eq!(find_index(&[9, 1, 9], 9), Some(0), "first of two matches");
    assert_eq!(find_index(&[4], 4), Some(0), "single element");
}

#[test]
fn find_index_misses_are_none() {
    assert_eq!(find_index(&[1, 2, 3], 10), None, "absent value");
    assert_eq!(find_index(&[], 1), None, "empty slice");
}

#[test]
fn first_even_works() {
    assert_eq!(first_even(&[1, 3, 4, 6]), Some(4), "4 comes first");
    assert_eq!(first_even(&[2]), Some(2), "single even");
    assert_eq!(first_even(&[-3, -8, 5]), Some(-8), "negative even");
    assert_eq!(first_even(&[1, 3, 5]), None, "all odd");
    assert_eq!(first_even(&[]), None, "empty");
}

#[test]
fn describe_formats_both_cases() {
    assert_eq!(describe(None), "none", "None");
    assert_eq!(describe(Some(7)), "got 7", "Some(7)");
    assert_eq!(describe(Some(-3)), "got -3", "Some(-3)");
    assert_eq!(describe(Some(0)), "got 0", "zero is still a value");
}

#[test]
fn double_or_zero_handles_none() {
    assert_eq!(double_or_zero(Some(21)), 42, "Some(21)");
    assert_eq!(double_or_zero(Some(-4)), -8, "Some(-4)");
    assert_eq!(double_or_zero(Some(0)), 0, "Some(0)");
    assert_eq!(double_or_zero(None), 0, "None");
}

#[test]
fn safe_div_divides_or_errors() {
    assert_eq!(safe_div(10, 2), Ok(5), "10 / 2");
    assert_eq!(safe_div(7, 2), Ok(3), "integer division");
    assert_eq!(safe_div(-9, 3), Ok(-3), "negative");
    assert_eq!(safe_div(0, 5), Ok(0), "zero numerator");
    assert_eq!(safe_div(5, 0), Err("division by zero".to_string()), "zero divisor");
    assert_eq!(safe_div(0, 0), Err("division by zero".to_string()), "0 / 0");
}

#[test]
fn parse_age_accepts_valid() {
    assert_eq!(parse_age("0"), Ok(0), "0");
    assert_eq!(parse_age("42"), Ok(42), "42");
    assert_eq!(parse_age("255"), Ok(255), "255");
}

#[test]
fn parse_age_rejects_invalid() {
    let bad = "not a valid age".to_string();
    for s in ["abc", "", "-1", "256", "1000", "4 2", "3.5"] {
        assert_eq!(parse_age(s), Err(bad.clone()), "parse_age({:?}) should be an error", s);
    }
}
