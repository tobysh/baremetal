fn rl(m: &str, p: &str, v: &str) -> Option<RequestLine> {
    Some(RequestLine { method: m.to_string(), path: p.to_string(), version: v.to_string() })
}

#[test]
fn parses_a_simple_get() {
    assert_eq!(parse_request_line("GET / HTTP/1.1"), rl("GET", "/", "HTTP/1.1"));
}

#[test]
fn parses_a_post_with_a_longer_path() {
    assert_eq!(parse_request_line("POST /notes/42 HTTP/1.0"), rl("POST", "/notes/42", "HTTP/1.0"));
}

#[test]
fn keeps_the_case_of_each_part() {
    assert_eq!(parse_request_line("get /Index HTTP/2"), rl("get", "/Index", "HTTP/2"));
}

#[test]
fn empty_line_is_none() {
    assert_eq!(parse_request_line(""), None, "an empty line has no parts");
}

#[test]
fn too_few_parts_is_none() {
    assert_eq!(parse_request_line("GET /"), None);
    assert_eq!(parse_request_line("GET"), None);
}

#[test]
fn too_many_parts_is_none() {
    assert_eq!(parse_request_line("GET / HTTP/1.1 extra"), None);
}

#[test]
fn doubled_space_is_none() {
    assert_eq!(parse_request_line("GET  / HTTP/1.1"), None, "split on single spaces, so this has 4 parts");
}

#[test]
fn version_must_start_with_http_slash() {
    assert_eq!(parse_request_line("GET / FTP/1.1"), None);
    assert_eq!(parse_request_line("GET / 1.1"), None);
}
