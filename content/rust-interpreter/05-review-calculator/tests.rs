fn s(x: &str) -> String {
    x.to_string()
}

#[test]
fn calc_success() {
    assert_eq!(calc("1 + 2 * 3"), Ok(Value::Int(7)), "arithmetic");
    assert_eq!(calc("(4 + 4) / 2 - 1"), Ok(Value::Int(3)), "parens");
    assert_eq!(calc("2 < 3"), Ok(Value::Bool(true)), "a boolean result");
    assert_eq!(calc("  -5  "), Ok(Value::Int(-5)), "whitespace");
}

#[test]
fn calc_parse_errors() {
    match calc("1 +") {
        Err(Error::Parse(p)) => assert_eq!(p.pos, 3, "parse error position"),
        other => panic!("expected a parse error, got {:?}", other),
    }
    assert!(matches!(calc("1 $ 2"), Err(Error::Parse(_))), "lexer errors count as parse errors");
    assert!(matches!(calc(""), Err(Error::Parse(_))), "empty input is a parse error");
}

#[test]
fn calc_runtime_errors() {
    assert_eq!(calc("1 / 0"), Err(Error::Runtime(RuntimeError::DivByZero)), "division by zero");
    assert_eq!(calc("y"), Err(Error::Runtime(RuntimeError::Undefined(s("y")))), "undefined name");
}

#[test]
fn lines_numbering_and_blank_lines() {
    assert_eq!(calc_lines(""), Vec::<String>::new(), "no lines, no output");
    assert_eq!(calc_lines("1\n2\n3"), vec![s("1: 1"), s("2: 2"), s("3: 3")], "three lines");
    assert_eq!(calc_lines("1\n\n   \n2"), vec![s("1: 1"), s("4: 2")], "blank lines are skipped but counted");
    assert_eq!(calc_lines("\n7\n"), vec![s("2: 7")], "leading and trailing blank lines");
}

#[test]
fn lines_with_errors() {
    assert_eq!(
        calc_lines("1 + 2\n10 / 0\n3 *\nq + 1\n4 $ 4"),
        vec![
            s("1: 3"),
            s("2: error: division by zero"),
            s("3: error: parse error at 3: unexpected end of input"),
            s("4: error: undefined variable 'q'"),
            s("5: error: parse error at 2: unexpected character '$'"),
        ],
        "each kind of error is reported in its own words"
    );
}

#[test]
fn lines_with_booleans_and_indentation() {
    assert_eq!(calc_lines("1 < 2\n   !true"), vec![s("1: true"), s("2: false")], "booleans print as true/false; leading spaces are fine");
    assert_eq!(calc_lines("   5 5"), vec![s("1: error: parse error at 5: unexpected Num(5)")], "positions count the leading spaces");
}

#[test]
fn total_adds_integers_only() {
    assert_eq!(calc_total("1\n2\n3"), Some(6), "plain sum");
    assert_eq!(calc_total("10\n1 < 2\n5\noops +\n1 / 0\n-3"), Some(12), "bools and errors are ignored");
    assert_eq!(calc_total("-5\n5"), Some(0), "a sum of zero is still Some");
}

#[test]
fn total_none_cases() {
    assert_eq!(calc_total(""), None, "nothing");
    assert_eq!(calc_total("1 < 2\nfoo"), None, "no integer results");
    assert_eq!(calc_total("9223372036854775807\n1"), None, "overflowing sum");
    assert_eq!(calc_total("9223372036854775807\n-1\n1"), Some(i64::MAX), "no overflow if the sum stays in range");
}
