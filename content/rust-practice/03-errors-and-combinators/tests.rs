#[test]
fn display_messages() {
    assert_eq!(PortError::Missing.to_string(), "no port configured", "Missing message");
    assert_eq!(PortError::Empty.to_string(), "port is empty", "Empty message");
    assert_eq!(PortError::OutOfRange(70000).to_string(), "port 70000 is out of range (1-65535)", "OutOfRange message");
}

#[test]
fn display_not_a_number_includes_inner_message() {
    let inner = "abc".parse::<i64>().unwrap_err();
    let msg = PortError::NotANumber(inner.clone()).to_string();
    assert_eq!(msg, format!("not a number: {}", inner), "NotANumber should wrap the inner message");
}

#[test]
fn from_parse_int_error() {
    let inner = "x".parse::<i64>().unwrap_err();
    let e: PortError = inner.clone().into();
    assert_eq!(e, PortError::NotANumber(inner), "From should build NotANumber");
}

#[test]
fn parse_port_valid() {
    assert_eq!(parse_port("8080"), Ok(8080), "8080 is valid");
    assert_eq!(parse_port("  443 \n"), Ok(443), "whitespace is trimmed");
    assert_eq!(parse_port("1"), Ok(1), "1 is the lowest valid port");
    assert_eq!(parse_port("65535"), Ok(65535), "65535 is the highest valid port");
}

#[test]
fn parse_port_empty() {
    assert_eq!(parse_port(""), Err(PortError::Empty), "empty string");
    assert_eq!(parse_port("   "), Err(PortError::Empty), "only spaces");
}

#[test]
fn parse_port_not_a_number() {
    match parse_port("http") {
        Err(PortError::NotANumber(_)) => {}
        other => panic!("expected NotANumber, got {:?}", other),
    }
}

#[test]
fn parse_port_out_of_range() {
    assert_eq!(parse_port("0"), Err(PortError::OutOfRange(0)), "0 is too low");
    assert_eq!(parse_port("65536"), Err(PortError::OutOfRange(65536)), "65536 is too high");
    assert_eq!(parse_port("-5"), Err(PortError::OutOfRange(-5)), "negative");
}

#[test]
fn lookup_finds_values() {
    let cfg = "host = example\nport=8080\nmode=fast";
    assert_eq!(lookup(cfg, "host"), Some("example"), "key and value are trimmed");
    assert_eq!(lookup(cfg, "mode"), Some("fast"), "last line");
}

#[test]
fn lookup_missing_and_junk_lines() {
    let cfg = "just some text\nport=1\nport=2";
    assert_eq!(lookup(cfg, "nope"), None, "absent key");
    assert_eq!(lookup(cfg, "port"), Some("1"), "the first match wins and junk lines are skipped");
}

#[test]
fn port_from_config_ok() {
    assert_eq!(port_from_config("a=b\nport=3000"), Ok(3000), "a good config");
}

#[test]
fn port_from_config_errors() {
    assert_eq!(port_from_config("a=b"), Err(PortError::Missing), "no port line");
    assert_eq!(port_from_config("port="), Err(PortError::Empty), "empty value");
    assert_eq!(port_from_config("port=99999"), Err(PortError::OutOfRange(99999)), "too big");
}

#[test]
fn halve_even_cases() {
    assert_eq!(halve_even(Some(10)), Some(5), "10 halves to 5");
    assert_eq!(halve_even(Some(7)), None, "odd gives None");
    assert_eq!(halve_even(None), None, "None stays None");
    assert_eq!(halve_even(Some(0)), Some(0), "zero is even");
    assert_eq!(halve_even(Some(-4)), Some(-2), "negative evens");
}
