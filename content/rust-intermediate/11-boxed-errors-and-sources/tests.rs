#[test]
fn read_pair_ok() {
    assert_eq!(read_pair("3,4").unwrap(), (3, 4), "simple pair");
    assert_eq!(read_pair(" -10 , 25 ").unwrap(), (-10, 25), "padding and a negative");
}

#[test]
fn read_pair_missing_comma() {
    let e = read_pair("34").unwrap_err();
    assert_eq!(e.to_string(), "expected 'a,b'", "message for a missing comma");
    assert!(!is_int_error(e.as_ref()), "that error is not a ParseIntError");
}

#[test]
fn read_pair_bad_number_keeps_the_parse_error() {
    let e = read_pair("1,x").unwrap_err();
    assert_eq!(e.to_string(), "invalid digit found in string", "the parse error's own message");
    assert!(is_int_error(e.as_ref()), "the Box still holds a ParseIntError");
    assert!(read_pair("a,2").is_err(), "first number bad");
}

#[test]
fn load_ok_and_err() {
    assert_eq!(load("cfg", " 42 ").unwrap(), 42, "valid");
    let e = load("cfg", "4x2").unwrap_err();
    assert_eq!(e.to_string(), "could not load 'cfg'", "the wrapper message names the entry");
    assert_eq!(e.name, "cfg", "the name is stored");
}

#[test]
fn source_is_reported() {
    let e = load("limits", "");
    let e = e.unwrap_err();
    let src = e.source().expect("a LoadError must report its cause");
    assert_eq!(src.to_string(), "cannot parse integer from empty string", "the cause is the ParseIntError");
}

#[test]
fn chain_walks_all_sources() {
    let e = load("port", "http").unwrap_err();
    assert_eq!(chain(&e), vec!["could not load 'port'", "invalid digit found in string"], "wrapper then cause");
    let p = "x".parse::<i32>().unwrap_err();
    assert_eq!(chain(&p), vec!["invalid digit found in string"], "a lone error has a chain of one");
}

#[test]
fn load_all_cases() {
    assert_eq!(load_all(&[("a", "1"), ("b", "2"), ("c", "-3")]).unwrap(), vec![1, 2, -3], "all valid");
    assert_eq!(load_all(&[]).unwrap(), Vec::<i32>::new(), "empty");
    let e = load_all(&[("a", "1"), ("b", "two"), ("c", "z")]).unwrap_err();
    assert_eq!(chain(e.as_ref()), vec!["could not load 'b'", "invalid digit found in string"], "the first failure, with its cause");
}

#[test]
fn is_int_error_distinguishes_types() {
    let p = "x".parse::<i32>().unwrap_err();
    assert!(is_int_error(&p), "a ParseIntError");
    let f = "x".parse::<f64>().unwrap_err();
    assert!(!is_int_error(&f), "a ParseFloatError is a different type");
    let l = load("a", "q").unwrap_err();
    assert!(!is_int_error(&l), "the wrapper itself is not a ParseIntError");
}
