fn live() -> isize {
    LIVE.with(|l| l.get())
}

#[test]
fn bump_adds_the_step() {
    unsafe {
        let c = counter_new(5);
        assert!(!c.is_null());
        assert_eq!(counter_total(c), 0);
        assert_eq!(counter_bump(c), 5);
        assert_eq!(counter_bump(c), 10);
        assert_eq!(counter_total(c), 10);
        counter_free(c);
    }
}

#[test]
fn handles_are_independent() {
    unsafe {
        let a = counter_new(1);
        let b = counter_new(100);
        for _ in 0..3 {
            counter_bump(a);
        }
        counter_bump(b);
        assert_eq!(counter_total(a), 3);
        assert_eq!(counter_total(b), 100);
        counter_free(a);
        assert_eq!(counter_total(b), 100, "b survives a's free");
        counter_free(b);
    }
}

#[test]
fn step_wraps_instead_of_panicking() {
    unsafe {
        let c = counter_new(u32::MAX);
        let mut want = 0u64;
        for _ in 0..3 {
            want = want.wrapping_add(u32::MAX as u64);
            assert_eq!(counter_bump(c), want);
        }
        counter_free(c);
        let z = counter_new(0);
        assert_eq!(counter_bump(z), 0);
        counter_free(z);
    }
}

#[test]
fn null_handles_are_harmless() {
    unsafe {
        assert_eq!(counter_bump(std::ptr::null_mut()), 0);
        assert_eq!(counter_total(std::ptr::null()), 0);
        counter_free(std::ptr::null_mut());
        assert_eq!(counter_into_inner(std::ptr::null_mut()), 0);
    }
    assert_eq!(live(), 0);
}

#[test]
fn every_handle_is_freed_exactly_once() {
    assert_eq!(live(), 0);
    unsafe {
        let hs: Vec<*mut Counter> = (1..=6).map(counter_new).collect();
        assert_eq!(live(), 6, "counters must be created with Counter::new, once each");
        for (i, &h) in hs.iter().enumerate() {
            for _ in 0..i {
                counter_bump(h);
            }
        }
        // free half, consume half
        for (i, &h) in hs.iter().enumerate() {
            if i % 2 == 0 {
                counter_free(h);
            } else {
                let want = (i as u64 + 1) * i as u64;
                assert_eq!(counter_into_inner(h), want, "handle {i}");
            }
        }
    }
    assert_eq!(live(), 0, "leaked or over-freed handles");
}
