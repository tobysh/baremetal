#[test]
fn reason_phrases_for_known_codes() {
    assert_eq!(reason_phrase(200), "OK");
    assert_eq!(reason_phrase(201), "Created");
    assert_eq!(reason_phrase(400), "Bad Request");
    assert_eq!(reason_phrase(404), "Not Found");
    assert_eq!(reason_phrase(405), "Method Not Allowed");
    assert_eq!(reason_phrase(500), "Internal Server Error");
}

#[test]
fn unknown_codes_say_unknown() {
    assert_eq!(reason_phrase(418), "Unknown");
    assert_eq!(reason_phrase(0), "Unknown");
}

#[test]
fn simple_ok_response_is_exact() {
    assert_eq!(
        build_response(200, "text/plain", "hello"),
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\nConnection: close\r\n\r\nhello"
    );
}

#[test]
fn not_found_response_uses_its_reason() {
    assert_eq!(
        build_response(404, "text/html", "<p>no</p>"),
        "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 9\r\nConnection: close\r\n\r\n<p>no</p>"
    );
}

#[test]
fn empty_body_has_zero_length() {
    assert_eq!(
        build_response(201, "text/plain", ""),
        "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
    );
}

#[test]
fn content_length_counts_bytes_not_characters() {
    let r = build_response(200, "text/plain", "héllo");
    assert!(r.contains("Content-Length: 6\r\n"), "héllo is 6 bytes in UTF-8, got: {:?}", r);
    assert!(r.ends_with("\r\n\r\nhéllo"));
}

#[test]
fn multibyte_emoji_body_length() {
    let r = build_response(200, "text/plain", "日本");
    assert!(r.contains("Content-Length: 6\r\n"), "two 3-byte characters, got: {:?}", r);
}

#[test]
fn unknown_status_still_builds() {
    let r = build_response(418, "text/plain", "x");
    assert!(r.starts_with("HTTP/1.1 418 Unknown\r\n"), "got: {:?}", r);
}
