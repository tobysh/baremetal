#[test]
fn parses_method_and_path() {
    let r = parse_request("GET /hello HTTP/1.1\r\nHost: x\r\n\r\n").expect("valid request should parse");
    assert_eq!(r.method, "GET");
    assert_eq!(r.path, "/hello");
}

#[test]
fn header_names_are_lowercased_and_values_trimmed() {
    let r = parse_request("GET / HTTP/1.1\r\nContent-Length:   5  \r\nHOST: example.com\r\n\r\n").unwrap();
    assert_eq!(r.headers.get("content-length").map(|s| s.as_str()), Some("5"));
    assert_eq!(r.headers.get("host").map(|s| s.as_str()), Some("example.com"));
    assert_eq!(r.headers.len(), 2);
}

#[test]
fn value_may_contain_colons() {
    let r = parse_request("GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n").unwrap();
    assert_eq!(r.headers.get("host").map(|s| s.as_str()), Some("localhost:8080"), "only split on the first colon");
}

#[test]
fn body_is_everything_after_the_blank_line() {
    let r = parse_request("POST /n HTTP/1.1\r\nContent-Length: 11\r\n\r\nhello\r\nworld").unwrap();
    assert_eq!(r.body, "hello\r\nworld", "the body may itself contain CRLF");
}

#[test]
fn no_body_gives_empty_string() {
    let r = parse_request("GET / HTTP/1.1\r\nHost: x\r\n\r\n").unwrap();
    assert_eq!(r.body, "");
}

#[test]
fn no_blank_line_means_empty_body() {
    let r = parse_request("GET / HTTP/1.1\r\nHost: x").unwrap();
    assert_eq!(r.body, "");
    assert_eq!(r.headers.len(), 1);
}

#[test]
fn request_line_only_works() {
    let r = parse_request("GET /only HTTP/1.1\r\n\r\n").unwrap();
    assert_eq!(r.path, "/only");
    assert!(r.headers.is_empty());
}

#[test]
fn later_duplicate_header_wins() {
    let r = parse_request("GET / HTTP/1.1\r\nX-A: 1\r\nx-a: 2\r\n\r\n").unwrap();
    assert_eq!(r.headers.get("x-a").map(|s| s.as_str()), Some("2"));
}

#[test]
fn empty_input_is_an_error() {
    assert!(parse_request("").is_err(), "no request line");
    assert!(parse_request("\r\n\r\n").is_err(), "no request line before the blank line");
}

#[test]
fn bad_request_line_is_an_error() {
    assert!(parse_request("BOGUS\r\nHost: x\r\n\r\n").is_err());
}

#[test]
fn header_without_colon_is_an_error() {
    assert!(parse_request("GET / HTTP/1.1\r\nthis is not a header\r\n\r\n").is_err());
}
