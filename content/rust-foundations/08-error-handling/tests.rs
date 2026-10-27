#[test]
fn sums_simple_list() {
    assert_eq!(parse_sum("1,2,3"), Ok(6), "parse_sum(\"1,2,3\") should be Ok(6)");
}

#[test]
fn trims_whitespace_around_items() {
    assert_eq!(parse_sum(" 10 ,  20,30 "), Ok(60), "parse_sum(\" 10 ,  20,30 \") should be Ok(60)");
}

#[test]
fn single_number() {
    assert_eq!(parse_sum("42"), Ok(42), "parse_sum(\"42\") should be Ok(42)");
}

#[test]
fn negative_numbers_are_allowed() {
    assert_eq!(parse_sum("-5, 3, -1"), Ok(-3), "parse_sum(\"-5, 3, -1\") should be Ok(-3)");
}

#[test]
fn empty_input_is_zero() {
    assert_eq!(parse_sum(""), Ok(0), "parse_sum(\"\") should be Ok(0)");
}

#[test]
fn whitespace_only_input_is_zero() {
    assert_eq!(parse_sum("   "), Ok(0), "parse_sum(\"   \") should be Ok(0)");
}

#[test]
fn large_values_fit_in_i64() {
    assert_eq!(parse_sum("3000000000, 3000000000"), Ok(6000000000), "values above i32 range should add correctly");
}

#[test]
fn invalid_item_is_reported_by_name() {
    let r = parse_sum("1, abc, 3");
    assert!(r.is_err(), "parse_sum(\"1, abc, 3\") should be an Err");
    let msg = r.unwrap_err();
    assert!(msg.contains("abc"), "the error should mention \"abc\", got {:?}", msg);
}

#[test]
fn invalid_last_item_is_an_error() {
    let msg = parse_sum("5,x9").unwrap_err();
    assert!(msg.contains("x9"), "the error should mention \"x9\", got {:?}", msg);
}

#[test]
fn empty_item_between_commas_is_an_error() {
    assert!(parse_sum("1,,2").is_err(), "parse_sum(\"1,,2\") should be an Err because one item is empty");
}
