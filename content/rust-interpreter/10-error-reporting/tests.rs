#[test]
fn positions_count_lines_and_characters() {
    for (src, pos, want) in [
        ("", 0, (1, 1)), ("abc", 0, (1, 1)), ("abc", 2, (1, 3)),
        ("abc", 3, (1, 4)), ("abc", 100, (1, 4)),
        ("a\nbc", 2, (2, 1)), ("a\nbc", 4, (2, 3)),
        ("a\n", 2, (2, 1)), ("\n\n", 2, (3, 1)),
        ("éλ\nx", 4, (1, 3)), ("éλ\nx", 5, (2, 1)),
        ("éλ\nx", 6, (2, 2)), ("x\tq", 2, (1, 3)),
    ] { assert_eq!(line_col(src, pos), want, "source {src:?}, byte {pos}"); }
}

#[test]
fn syntax_report_has_source_and_caret() {
    let src = "let x = 1;\nprint (2 + );";
    let err = Error::Parse(ParseError::new("unexpected RParen", src.find(')').unwrap()));
    assert_eq!(render_error(src, &err),
        "error: unexpected RParen\n  at line 2, column 12\n2 | print (2 + );\n  |            ^");
    let src = "# é\n$";
    assert_eq!(report(src),
        "error: unexpected character '$'\n  at line 2, column 1\n2 | $\n  | ^");
}

#[test]
fn wide_gutters_and_empty_eof_line() {
    let src = format!("{}bad", "\n".repeat(11));
    let err = Error::Parse(ParseError::new("example", 13));
    assert_eq!(render_error(&src, &err),
        "error: example\n  at line 12, column 3\n12 | bad\n   |   ^");
    assert_eq!(render_error("x\n", &Error::Parse(ParseError::new("missing", 2))),
        "error: missing\n  at line 2, column 1\n2 | \n  | ^");
    assert_eq!(render_error("éx", &Error::Parse(ParseError::new("unicode", 2))),
        "error: unicode\n  at line 1, column 2\n1 | éx\n  |  ^");
}

#[test]
fn runtime_errors_use_their_display() {
    for r in [RuntimeError::DivByZero, RuntimeError::Overflow,
        RuntimeError::Undefined("absent".into()), RuntimeError::NotCallable,
        RuntimeError::Arity { expected: 2, got: 0 },
        RuntimeError::TypeMismatch("example".into())] {
        assert_eq!(render_error("unrelated", &Error::Runtime(r.clone())), format!("runtime error: {r}"));
    }
}

#[test]
fn reports_run_real_programs() {
    for n in [-9, 0, 7, 31] {
        let src = format!("let x = {n}; print x * x; print x < 0;");
        assert_eq!(report(&src), format!("{}\n{}", n * n, n < 0));
    }
    assert_eq!(report(""), "");
    assert_eq!(report("print 1; print 7 / 0;"), "runtime error: division by zero");
    assert_eq!(report("print absent;"), "runtime error: undefined variable 'absent'");
    let src = "print 2";
    assert_eq!(report(src), "error: expected Semi, found end of input\n  at line 1, column 8\n1 | print 2\n  |        ^");
}
