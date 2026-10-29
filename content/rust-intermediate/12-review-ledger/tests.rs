fn bal(text: &str) -> Vec<(String, i64)> {
    apply(text).expect("this ledger is valid").into_iter().collect()
}

#[test]
fn replays_changes() {
    let v = bal("alice:+50\nbob:+20\nalice:-30\nbob:5");
    assert_eq!(v, vec![("alice".to_string(), 20), ("bob".to_string(), 25)], "alice 20, bob 25");
}

#[test]
fn zero_accounts_and_blank_lines_are_kept_or_skipped_correctly() {
    let v = bal("\n  zed : 10 \n\n zed:-10\n");
    assert_eq!(v, vec![("zed".to_string(), 0)], "an account at 0 is still listed");
    assert!(apply("").unwrap().is_empty(), "empty ledger");
}

#[test]
fn syntax_errors_count_blank_lines() {
    assert_eq!(apply("a:1\n\nbroken"), Err(LedgerError::Syntax(3)), "line 3 has no colon");
    assert_eq!(apply(":5"), Err(LedgerError::Syntax(1)), "empty name");
}

#[test]
fn amount_errors_keep_their_cause() {
    let e = apply("a:1\nb:ten").unwrap_err();
    match &e {
        LedgerError::Amount { line, .. } => assert_eq!(*line, 2, "the bad amount is on line 2"),
        other => panic!("expected an Amount error, got {other:?}"),
    }
    let cause = e.source().expect("Amount must report its ParseIntError");
    assert_eq!(cause.to_string(), "invalid digit found in string", "the cause");
    assert!(matches!(apply("a:"), Err(LedgerError::Amount { line: 1, .. })), "an empty amount is a bad amount");
}

#[test]
fn overdrafts() {
    assert_eq!(apply("alice:+10\nalice:-15"), Err(LedgerError::Overdraft { name: "alice".to_string(), balance: -5 }), "alice goes to -5");
    assert_eq!(apply("bob:-1"), Err(LedgerError::Overdraft { name: "bob".to_string(), balance: -1 }), "a new account cannot start negative");
    assert!(apply("a:+5\na:-5").is_ok(), "exactly zero is allowed");
}

#[test]
fn first_error_wins() {
    assert_eq!(apply("a:-1\nbroken"), Err(LedgerError::Overdraft { name: "a".to_string(), balance: -1 }), "the overdraft comes first");
}

#[test]
fn display_and_source_for_the_others() {
    assert_eq!(LedgerError::Syntax(3).to_string(), "line 3: expected name:amount", "Syntax");
    assert_eq!(LedgerError::Overdraft { name: "bob".to_string(), balance: -5 }.to_string(), "bob would go to -5", "Overdraft");
    assert!(LedgerError::Syntax(1).source().is_none(), "Syntax has no source");
    assert!(LedgerError::Overdraft { name: "x".to_string(), balance: -1 }.source().is_none(), "Overdraft has no source");
    let e = apply("q:z").unwrap_err();
    assert_eq!(e.to_string(), "line 1: bad amount (invalid digit found in string)", "Amount message includes the cause");
}

#[test]
fn reports() {
    assert_eq!(report("bob:+2\nalice:+7"), "alice=7, bob=2", "sorted by name");
    assert_eq!(report("a:+1\nb:-4"), "error: b would go to -4", "an overdraft report");
    assert_eq!(report("oops"), "error: line 1: expected name:amount", "a syntax report");
    assert_eq!(report(""), "", "an empty ledger reports nothing");
}
