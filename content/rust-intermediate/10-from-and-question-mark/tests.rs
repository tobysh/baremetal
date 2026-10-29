#[test]
fn from_wraps_the_original_error() {
    let ie = "x".parse::<i64>().unwrap_err();
    assert_eq!(AppError::from(ie.clone()), AppError::BadInt(ie), "From<ParseIntError>");
    let fe = "x".parse::<f64>().unwrap_err();
    assert_eq!(AppError::from(fe.clone()), AppError::BadFloat(fe), "From<ParseFloatError>");
}

#[test]
fn display_messages() {
    let ie = "abc".parse::<i64>().unwrap_err();
    assert_eq!(AppError::BadInt(ie).to_string(), "bad integer: invalid digit found in string", "BadInt");
    let fe = "abc".parse::<f64>().unwrap_err();
    assert_eq!(AppError::BadFloat(fe).to_string(), "bad float: invalid float literal", "BadFloat");
    assert_eq!(AppError::Negative(-7).to_string(), "negative value: -7", "Negative");
}

#[test]
fn parse_scaled_success() {
    assert_eq!(parse_scaled("4", "2.5"), Ok(10.0), "4 * 2.5");
    assert_eq!(parse_scaled(" 3 ", " 0.5 "), Ok(1.5), "padded inputs");
    assert_eq!(parse_scaled("0", "123.4"), Ok(0.0), "zero count");
    assert_eq!(parse_scaled("10", "-1.5"), Ok(-15.0), "a negative factor is allowed");
}

#[test]
fn parse_scaled_errors() {
    assert!(matches!(parse_scaled("x", "1.0"), Err(AppError::BadInt(_))), "bad count");
    assert!(matches!(parse_scaled("1", "y"), Err(AppError::BadFloat(_))), "bad factor");
    assert_eq!(parse_scaled("-3", "1.0"), Err(AppError::Negative(-3)), "negative count");
    assert_eq!(parse_scaled("-3", "not a float"), Err(AppError::Negative(-3)), "the count is checked first");
    assert!(matches!(parse_scaled("1.5", "1"), Err(AppError::BadInt(_))), "the count must be an integer");
}

#[test]
fn total_scaled_cases() {
    assert_eq!(total_scaled(&[("2", "1.5"), ("4", "0.25")]), Ok(4.0), "3 + 1");
    assert_eq!(total_scaled(&[]), Ok(0.0), "empty");
    assert_eq!(total_scaled(&[("1", "1"), ("-1", "1"), ("x", "1")]), Err(AppError::Negative(-1)), "the first error is reported");
}

#[test]
fn collect_valid_splits_results() {
    let (vals, errs) = collect_valid(&[("3", "1.5"), ("x", "1"), ("-2", "1"), ("2", "q"), ("10", "2")]);
    assert_eq!(vals, vec![4.5, 20.0], "the valid results in order");
    assert_eq!(
        errs,
        vec!["bad integer: invalid digit found in string", "negative value: -2", "bad float: invalid float literal"],
        "the error messages in order"
    );
    let (v, e) = collect_valid(&[]);
    assert!(v.is_empty() && e.is_empty(), "nothing in, nothing out");
}
