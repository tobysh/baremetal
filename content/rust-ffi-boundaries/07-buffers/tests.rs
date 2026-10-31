fn call(n: i64, cap: usize) -> (isize, Vec<u8>) {
    let mut buf = vec![0xAAu8; 40];
    let r = unsafe { write_decimal(buf.as_mut_ptr(), cap, n) };
    (r, buf)
}

#[test]
fn fits_exactly_and_with_room() {
    for n in [0i64, 7, -7, 12345, -987654321, i64::MAX, i64::MIN] {
        let s = n.to_string();
        for cap in [s.len() + 1, s.len() + 5] {
            let (r, buf) = call(n, cap);
            assert_eq!(r, s.len() as isize, "n {n} cap {cap}");
            assert_eq!(&buf[..s.len()], s.as_bytes());
            assert_eq!(buf[s.len()], 0, "NUL terminator");
            assert!(buf[s.len() + 1..].iter().all(|&b| b == 0xAA), "bytes past the NUL must be untouched");
        }
    }
}

#[test]
fn truncates_but_still_terminates() {
    let (r, buf) = call(-12345, 4);
    assert_eq!(r, 6, "return the length needed, not the length written");
    assert_eq!(&buf[..4], b"-12\0");
    assert!(buf[4..].iter().all(|&b| b == 0xAA), "never write past cap");
    let (r, buf) = call(99, 1);
    assert_eq!(r, 2);
    assert_eq!(buf[0], 0);
    assert_eq!(buf[1], 0xAA);
}

#[test]
fn size_query_and_null_buffer() {
    unsafe {
        assert_eq!(write_decimal(std::ptr::null_mut(), 0, i64::MIN), 20);
        assert_eq!(write_decimal(std::ptr::null_mut(), 0, 0), 1);
        assert_eq!(write_decimal(std::ptr::null_mut(), 5, 1), -1, "null buffer with capacity is an error");
    }
    let (r, buf) = call(123, 0);
    assert_eq!(r, 3);
    assert!(buf.iter().all(|&b| b == 0xAA), "cap 0 writes nothing");
}

#[test]
fn two_call_pattern_round_trips() {
    for n in [0i64, -1, 1000000, i64::MIN, i64::MAX] {
        unsafe {
            let need = write_decimal(std::ptr::null_mut(), 0, n) as usize;
            let mut v = vec![1u8; need + 1];
            assert_eq!(write_decimal(v.as_mut_ptr(), v.len(), n) as usize, need);
            assert_eq!(std::str::from_utf8(&v[..need]).unwrap().parse::<i64>().unwrap(), n);
        }
    }
}

#[test]
fn dup_copies_independently() {
    unsafe {
        let mut src = *b"hello, boundary";
        let p = dup_bytes(src.as_ptr(), src.len());
        assert!(!p.is_null());
        assert_ne!(p as *const u8, src.as_ptr(), "must be a new allocation");
        src[0] = b'J';
        assert_eq!(std::slice::from_raw_parts(p, 15), b"hello, boundary", "copy unaffected by the source");
        *p = b'Y';
        assert_eq!(src[0], b'J');
        free_bytes(p, 15);
        let q = dup_bytes(src.as_ptr(), 1);
        assert_eq!(*q, b'J');
        free_bytes(q, 1);
    }
}

#[test]
fn empty_and_null_cases() {
    unsafe {
        assert!(dup_bytes(std::ptr::null(), 4).is_null());
        assert!(dup_bytes(b"x".as_ptr(), 0).is_null());
        free_bytes(std::ptr::null_mut(), 0);
        free_bytes(std::ptr::null_mut(), 9);
    }
}
