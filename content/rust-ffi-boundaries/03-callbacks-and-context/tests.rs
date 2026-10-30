struct Rec {
    seen: Vec<i32>,
    stop_after: usize,
}

extern "C" fn t_record(ctx: *mut c_void, v: i32) -> i32 {
    let r = unsafe { &mut *(ctx as *mut Rec) };
    r.seen.push(v);
    (r.seen.len() == r.stop_after) as i32
}

fn lcg(s: &mut u64) -> i32 {
    *s = s.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
    ((*s >> 33) as i32) % 1000 - 500
}

#[test]
fn visits_in_order_and_counts() {
    let xs = [5, -2, 9, 0];
    let mut r = Rec { seen: vec![], stop_after: 0 };
    let n = unsafe { for_each(xs.as_ptr(), xs.len(), t_record, &mut r as *mut Rec as *mut c_void) };
    assert_eq!(n, 4);
    assert_eq!(r.seen, xs);
}

#[test]
fn stops_when_callback_returns_non_zero() {
    let xs = [1, 2, 3, 4, 5];
    for stop in 1..=5usize {
        let mut r = Rec { seen: vec![], stop_after: stop };
        let n = unsafe { for_each(xs.as_ptr(), xs.len(), t_record, &mut r as *mut Rec as *mut c_void) };
        assert_eq!(n, stop as isize, "stop after {stop}");
        assert_eq!(r.seen, &xs[..stop], "no calls after stopping");
    }
}

#[test]
fn null_and_empty_inputs() {
    let mut r = Rec { seen: vec![], stop_after: 0 };
    let ctx = &mut r as *mut Rec as *mut c_void;
    unsafe {
        assert_eq!(for_each(std::ptr::null(), 0, t_record, ctx), 0);
        assert_eq!(for_each(std::ptr::null(), 3, t_record, ctx), -1);
        let empty: [i32; 0] = [];
        assert_eq!(for_each(empty.as_ptr(), 0, t_record, ctx), 0);
    }
    assert!(r.seen.is_empty(), "no callback may run for these inputs");
}

#[test]
fn stats_match_a_reference() {
    assert_eq!(stats_of(&[]), Stats { count: 0, sum: 0, max: None });
    assert_eq!(stats_of(&[-7]), Stats { count: 1, sum: -7, max: Some(-7) });
    assert_eq!(
        stats_of(&[i32::MAX, i32::MAX, i32::MAX]),
        Stats { count: 3, sum: 3 * i32::MAX as i64, max: Some(i32::MAX) },
        "sum must not overflow"
    );
    let mut s = 21u64;
    for len in [2usize, 5, 17, 60] {
        let xs: Vec<i32> = (0..len).map(|_| lcg(&mut s)).collect();
        let want = Stats { count: len, sum: xs.iter().map(|&x| x as i64).sum(), max: xs.iter().copied().max() };
        assert_eq!(stats_of(&xs), want, "len {len}");
    }
}

#[test]
fn first_negative_matches_position() {
    assert_eq!(first_negative(&[]), None);
    assert_eq!(first_negative(&[1, 2, 3]), None);
    assert_eq!(first_negative(&[-1, 2, 3]), Some(0));
    assert_eq!(first_negative(&[1, 2, -3, -4]), Some(2));
    let mut s = 8u64;
    for _ in 0..40 {
        let len = (lcg(&mut s).unsigned_abs() % 12) as usize;
        let xs: Vec<i32> = (0..len).map(|_| lcg(&mut s)).collect();
        assert_eq!(first_negative(&xs), xs.iter().position(|&x| x < 0), "{xs:?}");
    }
}
