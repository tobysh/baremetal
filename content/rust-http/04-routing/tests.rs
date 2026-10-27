fn r(m: &str, p: &str) -> (u16, String) {
    route(m, p)
}

#[test]
fn root_welcomes() {
    assert_eq!(r("GET", "/"), (200, "Welcome to Baremetal".to_string()));
}

#[test]
fn health_is_ok() {
    assert_eq!(r("GET", "/health"), (200, "ok".to_string()));
}

#[test]
fn echo_returns_the_text() {
    assert_eq!(r("GET", "/echo/hello"), (200, "hello".to_string()));
    assert_eq!(r("GET", "/echo/a-b_c"), (200, "a-b_c".to_string()));
}

#[test]
fn echo_with_empty_text_is_404() {
    assert_eq!(r("GET", "/echo/"), (404, "not found".to_string()));
    assert_eq!(r("GET", "/echo"), (404, "not found".to_string()));
}

#[test]
fn echo_with_extra_segments_is_404() {
    assert_eq!(r("GET", "/echo/a/b"), (404, "not found".to_string()));
}

#[test]
fn add_sums_two_numbers() {
    assert_eq!(r("GET", "/add/2/3"), (200, "5".to_string()));
    assert_eq!(r("GET", "/add/-4/10"), (200, "6".to_string()));
    assert_eq!(r("GET", "/add/0/0"), (200, "0".to_string()));
}

#[test]
fn add_handles_large_i64_values() {
    assert_eq!(r("GET", "/add/9000000000/9000000000"), (200, "18000000000".to_string()));
}

#[test]
fn add_with_a_bad_number_is_400() {
    assert_eq!(r("GET", "/add/x/3"), (400, "bad number".to_string()));
    assert_eq!(r("GET", "/add/3/y"), (400, "bad number".to_string()));
    assert_eq!(r("GET", "/add/1.5/2"), (400, "bad number".to_string()));
    assert_eq!(r("GET", "/add//2"), (400, "bad number".to_string()));
}

#[test]
fn add_with_wrong_segment_count_is_404() {
    assert_eq!(r("GET", "/add/1"), (404, "not found".to_string()));
    assert_eq!(r("GET", "/add/1/2/3"), (404, "not found".to_string()));
}

#[test]
fn unknown_get_path_is_404() {
    assert_eq!(r("GET", "/nope"), (404, "not found".to_string()));
    assert_eq!(r("GET", "/health/"), (404, "not found".to_string()));
}

#[test]
fn non_get_on_known_path_is_405() {
    assert_eq!(r("POST", "/"), (405, "method not allowed".to_string()));
    assert_eq!(r("POST", "/health"), (405, "method not allowed".to_string()));
    assert_eq!(r("DELETE", "/echo/x"), (405, "method not allowed".to_string()));
    assert_eq!(r("PUT", "/add/1/2"), (405, "method not allowed".to_string()));
    assert_eq!(r("get", "/health"), (405, "method not allowed".to_string()), "methods are case-sensitive");
}

#[test]
fn non_get_on_unknown_path_is_404() {
    assert_eq!(r("POST", "/nope"), (404, "not found".to_string()));
    assert_eq!(r("POST", "/echo/"), (404, "not found".to_string()));
}
