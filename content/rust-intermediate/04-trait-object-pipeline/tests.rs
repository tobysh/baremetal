#[test]
fn stages_individually() {
    assert_eq!(AddN(5).apply(7), Some(12), "add");
    assert_eq!(AddN(-5).apply(7), Some(2), "add negative");
    assert_eq!(MulN(3).apply(-4), Some(-12), "mul");
    assert_eq!(NonNeg.apply(0), Some(0), "zero is non-negative");
    assert_eq!(NonNeg.apply(-1), None, "negative rejected");
    assert_eq!(Clamp(10).apply(25), Some(10), "clamped");
    assert_eq!(Clamp(10).apply(4), Some(4), "under the cap");
}

#[test]
fn stage_names() {
    assert_eq!(AddN(5).name(), "add:5", "add name");
    assert_eq!(AddN(-2).name(), "add:-2", "negative arg name");
    assert_eq!(MulN(3).name(), "mul:3", "mul name");
    assert_eq!(NonNeg.name(), "nonneg", "nonneg name");
    assert_eq!(Clamp(30).name(), "clamp:30", "clamp name");
}

#[test]
fn checked_overflow() {
    assert_eq!(AddN(1).apply(i64::MAX), None, "add overflow");
    assert_eq!(MulN(2).apply(i64::MAX), None, "mul overflow");
    assert_eq!(MulN(2).apply(i64::MAX / 2), Some(i64::MAX - 1), "just fits");
}

#[test]
fn build_valid_specs() {
    let s = build_all(&["add:5", "mul:3"]).expect("both specs are valid");
    assert_eq!(run(&s, 7), Some(36), "(7 + 5) * 3");
    assert_eq!(run(&s, -5), Some(0), "(-5 + 5) * 3");
    let t = build_all(&["mul:3", "add:5", "clamp:30"]).expect("valid");
    assert_eq!(run(&t, 20), Some(30), "capped at 30");
    assert_eq!(run(&t, 2), Some(11), "under the cap");
}

#[test]
fn run_short_circuits() {
    let s = build_all(&["add:-10", "nonneg", "mul:2"]).unwrap();
    assert_eq!(run(&s, 5), None, "5 - 10 is negative so nonneg stops it");
    assert_eq!(run(&s, 12), Some(4), "12 - 10 = 2, doubled");
    let none: Vec<Box<dyn Stage>> = Vec::new();
    assert_eq!(run(&none, 99), Some(99), "no stages changes nothing");
}

#[test]
fn names_in_order() {
    let s = build_all(&["clamp:9", "add:-1", "nonneg", "mul:4"]).unwrap();
    assert_eq!(names(&s), vec!["clamp:9", "add:-1", "nonneg", "mul:4"], "names follow the specs");
}

#[test]
fn bad_specs() {
    assert!(build("bogus").is_none(), "unknown stage");
    assert!(build("add").is_none(), "missing argument");
    assert!(build("add:x").is_none(), "non-numeric argument");
    assert!(build("add:").is_none(), "empty argument");
    assert!(build("nonneg:3").is_none(), "nonneg takes no argument");
    assert!(build("").is_none(), "empty spec");
    assert!(build_all(&["add:1", "oops", "mul:2"]).is_none(), "one bad spec spoils the list");
    assert_eq!(build_all(&[]).map(|v| v.len()), Some(0), "an empty list is fine");
}
