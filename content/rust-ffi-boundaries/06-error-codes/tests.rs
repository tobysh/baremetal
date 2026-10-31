fn nine(x: i32) -> i32 {
    if x == 9 { panic!("nine is not allowed") }
    x * 2
}

fn oob(x: i32) -> i32 {
    let v = vec![1, 2, 3];
    v[x as usize]
}

fn parse(s: &[u8]) -> (i32, u8) {
    let mut out = 77u8;
    let c = unsafe { parse_u8(s.as_ptr(), s.len(), &mut out) };
    (c, out)
}

#[test]
fn div_codes_and_untouched_output() {
    let mut out = 1234;
    unsafe {
        assert_eq!(checked_div(7, 2, &mut out), OK);
        assert_eq!(out, 3);
        assert_eq!(checked_div(-7, 2, &mut out), OK);
        assert_eq!(out, -3, "truncates toward zero");
        out = 1234;
        assert_eq!(checked_div(5, 0, &mut out), BAD_INPUT);
        assert_eq!(checked_div(i32::MIN, -1, &mut out), OVERFLOW);
        assert_eq!(out, 1234, "unchanged on error");
        assert_eq!(checked_div(i32::MIN, 1, &mut out), OK);
        assert_eq!(out, i32::MIN);
        assert_eq!(checked_div(1, 1, std::ptr::null_mut()), NULL_ARG);
        assert_eq!(checked_div(1, 0, std::ptr::null_mut()), NULL_ARG, "null is checked first");
    }
}

#[test]
fn parse_accepts_valid_numbers() {
    for n in 0..=255u32 {
        assert_eq!(parse(n.to_string().as_bytes()), (OK, n as u8), "{n}");
    }
    assert_eq!(parse(b"0000255"), (OK, 255));
    assert_eq!(parse(b"007"), (OK, 7));
}

#[test]
fn parse_reports_each_failure_without_writing() {
    for bad in [&b""[..], b"-1", b"+5", b"12a", b" 5", b"5 ", b"1.5", b"\xff"] {
        assert_eq!(parse(bad), (BAD_INPUT, 77), "{bad:?}");
    }
    for big in [&b"256"[..], b"300", b"1000", b"99999999999999999999999999", b"0256"] {
        assert_eq!(parse(big), (OVERFLOW, 77), "{big:?}");
    }
    assert_eq!(parse(b"99999999999999999999999999x"), (BAD_INPUT, 77), "non-digit beats overflow");
}

#[test]
fn parse_null_arguments() {
    let mut out = 77u8;
    unsafe {
        assert_eq!(parse_u8(std::ptr::null(), 3, &mut out), NULL_ARG);
        assert_eq!(parse_u8(b"1".as_ptr(), 1, std::ptr::null_mut()), NULL_ARG);
        assert_eq!(parse_u8(std::ptr::null(), 0, &mut out), BAD_INPUT, "null with len 0 is an empty string");
    }
    assert_eq!(out, 77);
}

#[test]
fn panics_become_codes() {
    let mut out = -1;
    unsafe {
        assert_eq!(guarded(nine, 4, &mut out), OK);
        assert_eq!(out, 8);
        assert_eq!(guarded(nine, 9, &mut out), PANICKED);
        assert_eq!(out, 8, "unchanged after a panic");
        assert_eq!(guarded(oob, 1, &mut out), OK);
        assert_eq!(out, 2);
        assert_eq!(guarded(oob, 7, &mut out), PANICKED, "index out of bounds panics too");
        assert_eq!(guarded(nine, 1, std::ptr::null_mut()), NULL_ARG);
    }
}

#[test]
fn messages_and_results() {
    assert_eq!(status_message(OK), "ok");
    assert_eq!(status_message(NULL_ARG), "null argument");
    assert_eq!(status_message(BAD_INPUT), "bad input");
    assert_eq!(status_message(OVERFLOW), "overflow");
    assert_eq!(status_message(PANICKED), "panicked");
    assert_eq!(status_message(5), "unknown");
    assert_eq!(status_message(-4), "unknown");
    assert_eq!(into_result(OK), Ok(()));
    assert_eq!(into_result(OVERFLOW), Err(OVERFLOW));
    assert_eq!(into_result(42), Err(42));
}
