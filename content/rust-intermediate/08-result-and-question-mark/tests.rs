#[test]
fn parse_sum_cases() {
    assert_eq!(parse_sum("20", " 22 "), Ok(42), "whitespace is ignored");
    assert_eq!(parse_sum("-5", "3"), Ok(-2), "negatives");
    assert!(parse_sum("1", "x").is_err(), "second input bad");
    assert!(parse_sum("x", "1").is_err(), "first input bad");
    assert!(parse_sum("", "1").is_err(), "empty input");
}

#[test]
fn sum_all_cases() {
    assert_eq!(sum_all(&["1", "2", "3"]), Ok(6), "three numbers");
    assert_eq!(sum_all(&[]), Ok(0), "no numbers");
    assert_eq!(sum_all(&[" 10 ", "-4"]), Ok(6), "whitespace and a negative");
    assert!(sum_all(&["1", "two", "3"]).is_err(), "a bad item in the middle");
}

#[test]
fn first_even_finds_it() {
    assert_eq!(first_even(&["3", "7", "10", "12"]), Ok(Some(10)), "first even of several");
    assert_eq!(first_even(&["-4"]), Ok(Some(-4)), "negative even");
    assert_eq!(first_even(&["0"]), Ok(Some(0)), "zero is even");
}

#[test]
fn first_even_none() {
    assert_eq!(first_even(&["1", "3", "5"]), Ok(None), "all odd");
    assert_eq!(first_even(&[]), Ok(None), "empty");
}

#[test]
fn first_even_errors_only_when_reached() {
    assert_eq!(first_even(&["3", "10", "x"]), Ok(Some(10)), "the bad item is never reached");
    assert!(first_even(&["3", "x", "10"]).is_err(), "the bad item comes first");
    assert!(first_even(&["1", "3", "z"]).is_err(), "no even number, so the bad item is reached");
}

#[test]
fn parse_or_cases() {
    assert_eq!(parse_or("17", 0), 17, "valid");
    assert_eq!(parse_or(" 17 ", 0), 17, "padded");
    assert_eq!(parse_or("oops", 99), 99, "invalid uses the default");
    assert_eq!(parse_or("", -1), -1, "empty uses the default");
    assert_eq!(parse_or("99999999999", 5), 5, "overflow uses the default");
}

#[test]
fn describe_cases() {
    assert_eq!(describe("42"), "number 42", "valid number");
    assert_eq!(describe(" -7 "), "number -7", "padded negative");
    assert_eq!(describe("abc"), "error: invalid digit found in string", "bad digit");
    assert_eq!(describe(""), "error: cannot parse integer from empty string", "empty");
    assert_eq!(describe("99999999999"), "error: number too large to fit in target type", "overflow");
}
