fn msgs(v: &[&str]) -> Vec<String> {
    v.iter().map(|s| s.to_string()).collect()
}

#[test]
fn good_input_is_summed() {
    assert_eq!(parse_report(" 4, -9 ,10"), Ok(5), "4 - 9 + 10 is 5");
    assert_eq!(parse_report("7"), Ok(7), "one item");
}

#[test]
fn empty_input_is_zero() {
    assert_eq!(parse_report(""), Ok(0));
    assert_eq!(parse_report("   \t"), Ok(0));
}

#[test]
fn reports_every_bad_item_in_order() {
    assert_eq!(
        parse_report("1, x, , 4"),
        Err(msgs(&["item 2 is not a number: x", "item 3 is empty"])),
        "both problems must be reported, in order"
    );
    assert_eq!(
        parse_report("a,b,c"),
        Err(msgs(&["item 1 is not a number: a", "item 2 is not a number: b", "item 3 is not a number: c"])),
        "three bad items give three messages"
    );
}

#[test]
fn trailing_and_leading_commas_are_empty_items() {
    assert_eq!(parse_report("1,2,"), Err(msgs(&["item 3 is empty"])), "a trailing comma leaves an empty last item");
    assert_eq!(parse_report(",5"), Err(msgs(&["item 1 is empty"])), "a leading comma leaves an empty first item");
}

#[test]
fn too_big_for_i64_is_not_a_number() {
    assert_eq!(
        parse_report("1, 99999999999999999999, 2"),
        Err(msgs(&["item 2 is not a number: 99999999999999999999"])),
        "this value does not fit in an i64"
    );
}

#[test]
fn sum_overflow_is_reported_once() {
    let big = i64::MAX.to_string();
    assert_eq!(parse_report(&format!("{}, 1", big)), Err(msgs(&["sum overflows"])), "MAX + 1 overflows");
    let small = i64::MIN.to_string();
    assert_eq!(parse_report(&format!("{}, -1", small)), Err(msgs(&["sum overflows"])), "MIN - 1 overflows");
    assert_eq!(parse_report(&format!("{}, -1, 1", big)), Ok(i64::MAX), "MAX - 1 + 1 stays in range");
    assert_eq!(parse_report(&format!("{}, {}", big, small)), Ok(-1), "MAX + MIN is -1");
}

#[test]
fn item_errors_win_over_overflow() {
    let big = i64::MAX.to_string();
    assert_eq!(
        parse_report(&format!("{}, 1, nope", big)),
        Err(msgs(&["item 3 is not a number: nope"])),
        "do not mention the overflow when items are bad"
    );
}
