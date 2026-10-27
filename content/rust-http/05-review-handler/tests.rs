fn req_with(headers: &[(&str, &str)], body: &str) -> Request {
    let mut h = HashMap::new();
    for (k, v) in headers {
        h.insert(k.to_string(), v.to_string());
    }
    Request { method: "POST".to_string(), path: "/x".to_string(), headers: h, body: body.to_string() }
}

#[test]
fn no_content_length_is_fine() {
    assert!(body_matches_length(&req_with(&[], "hello")), "no header means nothing to check");
}

#[test]
fn matching_content_length_is_fine() {
    assert!(body_matches_length(&req_with(&[("content-length", "5")], "hello")), "5 matches \"hello\"");
    assert!(body_matches_length(&req_with(&[("content-length", "0")], "")), "0 matches an empty body");
}

#[test]
fn wrong_content_length_is_rejected() {
    assert!(!body_matches_length(&req_with(&[("content-length", "4")], "hello")), "4 does not match \"hello\"");
    assert!(!body_matches_length(&req_with(&[("content-length", "6")], "hello")), "6 does not match \"hello\"");
}

#[test]
fn unparseable_content_length_is_rejected() {
    for bad in ["abc", "-1", "", "5x", "1.5"] {
        assert!(!body_matches_length(&req_with(&[("content-length", bad)], "hello")), "content-length {:?} is not a valid length", bad);
    }
}

#[test]
fn content_length_counts_bytes_not_characters() {
    assert!(body_matches_length(&req_with(&[("content-length", "6")], "héllo")), "\"héllo\" is 6 bytes");
    assert!(!body_matches_length(&req_with(&[("content-length", "5")], "héllo")), "\"héllo\" is not 5 bytes");
}

#[test]
fn handle_routes_a_normal_get() {
    let raw = "GET /echo/hi HTTP/1.1\r\nHost: x\r\n\r\n";
    assert_eq!(handle(raw), build_response(200, "text/plain", "hi"), "GET /echo/hi should respond 200 hi");
}

#[test]
fn handle_passes_route_errors_through() {
    assert_eq!(handle("GET /nope HTTP/1.1\r\n\r\n"), build_response(404, "text/plain", "not found"), "unknown path should be 404");
    assert_eq!(handle("POST /health HTTP/1.1\r\n\r\n"), build_response(405, "text/plain", "method not allowed"), "POST /health should be 405");
    assert_eq!(handle("GET /add/x/1 HTTP/1.1\r\n\r\n"), build_response(400, "text/plain", "bad number"), "bad numbers should be 400");
}

#[test]
fn handle_does_arithmetic_through_route() {
    assert_eq!(handle("GET /add/2/3 HTTP/1.1\r\n\r\n"), build_response(200, "text/plain", "5"), "GET /add/2/3 should be 5");
}

#[test]
fn handle_rejects_unparseable_requests() {
    let bad = build_response(400, "text/plain", "bad request");
    assert_eq!(handle(""), bad, "an empty request should be 400");
    assert_eq!(handle("nonsense"), bad, "a request with no valid request line should be 400");
    assert_eq!(handle("GET / HTTP/1.1\r\nno colon here\r\n\r\n"), bad, "a header with no colon should be 400");
}

#[test]
fn handle_rejects_a_lying_content_length() {
    let bad = build_response(400, "text/plain", "bad request");
    assert_eq!(handle("POST /shout HTTP/1.1\r\nContent-Length: 10\r\n\r\nhi"), bad, "claiming 10 bytes with 2 should be 400");
    assert_eq!(handle("POST /shout HTTP/1.1\r\nContent-Length: abc\r\n\r\nhi"), bad, "a non-numeric length should be 400");
    assert_eq!(handle("GET /health HTTP/1.1\r\nContent-Length: 3\r\n\r\n"), bad, "a length of 3 with no body should be 400 on any route");
}

#[test]
fn shout_uppercases_the_body() {
    assert_eq!(
        handle("POST /shout HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello"),
        build_response(200, "text/plain", "HELLO"),
        "POST /shout with hello should respond HELLO"
    );
}

#[test]
fn shout_works_without_content_length_and_with_unicode() {
    assert_eq!(handle("POST /shout HTTP/1.1\r\n\r\nstraße"), build_response(200, "text/plain", "STRASSE"), "to_uppercase should be used");
    assert_eq!(handle("POST /shout HTTP/1.1\r\n\r\n"), build_response(200, "text/plain", ""), "an empty body shouts nothing");
}

#[test]
fn shout_is_post_only() {
    assert_eq!(handle("GET /shout HTTP/1.1\r\n\r\n"), build_response(404, "text/plain", "not found"), "GET /shout is just an unknown path");
}

#[test]
fn header_names_are_case_insensitive() {
    assert_eq!(
        handle("POST /shout HTTP/1.1\r\nCONTENT-LENGTH: 2\r\n\r\nhi"),
        build_response(200, "text/plain", "HI"),
        "CONTENT-LENGTH should be understood as content-length"
    );
}
