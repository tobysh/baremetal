struct Seen {
    items: Vec<(Vec<u8>, i64)>,
    stop_after: usize,
}

extern "C" fn t_collect(ctx: *mut c_void, e: *const Entry) -> i32 {
    let s = unsafe { &mut *(ctx as *mut Seen) };
    let e = unsafe { &*e };
    let k = unsafe { std::slice::from_raw_parts(e.key, e.key_len) }.to_vec();
    s.items.push((k, e.value));
    (s.items.len() == s.stop_after) as i32
}

fn lcg(s: &mut u64) -> u64 {
    *s = s.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
    *s >> 33
}

fn each(s: *const Store, stop_after: usize) -> (isize, Vec<(Vec<u8>, i64)>) {
    let mut seen = Seen { items: vec![], stop_after };
    let n = unsafe { kv_each(s, t_collect, &mut seen as *mut Seen as *mut c_void) };
    (n, seen.items)
}

#[test]
fn entry_layout_is_c_like() {
    assert_eq!(std::mem::size_of::<Entry>(), 24);
    assert_eq!(std::mem::offset_of!(Entry, key), 0);
    assert_eq!(std::mem::offset_of!(Entry, key_len), 8);
    assert_eq!(std::mem::offset_of!(Entry, value), 16);
}

#[test]
fn basic_add_get_remove() {
    unsafe {
        let s = kv_new();
        assert!(!s.is_null());
        let mut v = 0;
        assert_eq!(kv_get(s, b"a".as_ptr(), 1, &mut v), NOT_FOUND);
        assert_eq!(kv_add(s, b"a".as_ptr(), 1, 5, &mut v), OK);
        assert_eq!(v, 5);
        assert_eq!(kv_add(s, b"a".as_ptr(), 1, -8, &mut v), OK);
        assert_eq!(v, -3);
        assert_eq!(kv_add(s, b"a".as_ptr(), 1, 1, std::ptr::null_mut()), OK, "out may be null");
        assert_eq!(kv_get(s, b"a".as_ptr(), 1, &mut v), OK);
        assert_eq!(v, -2);
        assert_eq!(kv_get(s, b"a".as_ptr(), 1, std::ptr::null_mut()), OK);
        assert_eq!(kv_remove(s, b"a".as_ptr(), 1), OK);
        assert_eq!(kv_remove(s, b"a".as_ptr(), 1), NOT_FOUND);
        assert_eq!(kv_get(s, b"a".as_ptr(), 1, &mut v), NOT_FOUND);
        kv_free(s);
    }
}

#[test]
fn argument_errors() {
    unsafe {
        let s = kv_new();
        let mut v = 42;
        assert_eq!(kv_add(std::ptr::null_mut(), b"a".as_ptr(), 1, 1, &mut v), NULL_ARG);
        assert_eq!(kv_add(s, std::ptr::null(), 3, 1, &mut v), NULL_ARG);
        assert_eq!(kv_add(s, b"a".as_ptr(), 0, 1, &mut v), BAD_KEY);
        assert_eq!(kv_get(s, b"a".as_ptr(), 0, &mut v), BAD_KEY);
        assert_eq!(kv_get(std::ptr::null(), b"a".as_ptr(), 1, &mut v), NULL_ARG);
        assert_eq!(kv_remove(s, std::ptr::null(), 2), NULL_ARG);
        assert_eq!(kv_remove(std::ptr::null_mut(), b"k".as_ptr(), 1), NULL_ARG);
        assert_eq!(v, 42, "out untouched on error");
        assert_eq!(each(s, 0).0, 0, "failed calls must not create keys");
        assert_eq!(kv_each(std::ptr::null(), t_collect, std::ptr::null_mut()), -1);
        kv_free(s);
        kv_free(std::ptr::null_mut());
    }
}

#[test]
fn overflow_changes_nothing() {
    unsafe {
        let s = kv_new();
        let mut v = 7;
        assert_eq!(kv_add(s, b"big".as_ptr(), 3, i64::MAX, &mut v), OK);
        assert_eq!(kv_add(s, b"big".as_ptr(), 3, 1, &mut v), OVERFLOW);
        assert_eq!(v, i64::MAX);
        assert_eq!(kv_add(s, b"neg".as_ptr(), 3, i64::MIN, &mut v), OK);
        assert_eq!(kv_add(s, b"neg".as_ptr(), 3, -1, &mut v), OVERFLOW);
        assert_eq!(kv_get(s, b"neg".as_ptr(), 3, &mut v), OK);
        assert_eq!(v, i64::MIN);
        kv_free(s);
    }
}

#[test]
fn iteration_is_sorted_and_stoppable() {
    unsafe {
        let s = kv_new();
        for (k, d) in [(&b"pear"[..], 1), (b"apple", 2), (b"app", 3), (b"zed", 4), (b"\x00\xff", 5)] {
            assert_eq!(kv_add(s, k.as_ptr(), k.len(), d, std::ptr::null_mut()), OK);
        }
        let (n, all) = each(s, 0);
        assert_eq!(n, 5);
        let keys: Vec<&[u8]> = all.iter().map(|(k, _)| k.as_slice()).collect();
        assert_eq!(keys, vec![&b"\x00\xff"[..], b"app", b"apple", b"pear", b"zed"]);
        assert_eq!(all[2].1, 2);
        let (n, part) = each(s, 2);
        assert_eq!(n, 2);
        assert_eq!(part, all[..2].to_vec());
        kv_free(s);
    }
}

#[test]
fn matches_a_model_over_random_operations() {
    use std::collections::BTreeMap;
    let keys: [&[u8]; 6] = [b"a", b"b", b"ab", b"\x00", b"long key with spaces", b"\xff\xfe"];
    let deltas = [0i64, 1, -1, 1000, i64::MAX, i64::MIN, i64::MAX / 2, -(i64::MAX / 2)];
    for seed in 1..=6u64 {
        let mut rng = seed * 7919;
        let mut model: BTreeMap<Vec<u8>, i64> = BTreeMap::new();
        unsafe {
            let s = kv_new();
            for step in 0..400 {
                let k = keys[lcg(&mut rng) as usize % keys.len()];
                match lcg(&mut rng) % 4 {
                    0 | 1 => {
                        let d = deltas[lcg(&mut rng) as usize % deltas.len()];
                        let cur = model.get(k).copied().unwrap_or(0);
                        let mut out = -12345i64;
                        let code = kv_add(s, k.as_ptr(), k.len(), d, &mut out);
                        match cur.checked_add(d) {
                            Some(v) => {
                                model.insert(k.to_vec(), v);
                                assert_eq!((code, out), (OK, v), "seed {seed} step {step} add");
                            }
                            None => assert_eq!((code, out), (OVERFLOW, -12345), "seed {seed} step {step} overflow"),
                        }
                    }
                    2 => {
                        let mut out = -12345i64;
                        let code = kv_get(s, k.as_ptr(), k.len(), &mut out);
                        match model.get(k) {
                            Some(&v) => assert_eq!((code, out), (OK, v), "seed {seed} step {step} get"),
                            None => assert_eq!((code, out), (NOT_FOUND, -12345), "seed {seed} step {step} get missing"),
                        }
                    }
                    _ => {
                        let code = kv_remove(s, k.as_ptr(), k.len());
                        let want = if model.remove(k).is_some() { OK } else { NOT_FOUND };
                        assert_eq!(code, want, "seed {seed} step {step} remove");
                    }
                }
                if step % 50 == 49 {
                    let (n, all) = each(s, 0);
                    let want: Vec<(Vec<u8>, i64)> = model.iter().map(|(k, &v)| (k.clone(), v)).collect();
                    assert_eq!(n as usize, want.len());
                    assert_eq!(all, want, "seed {seed} step {step} iteration");
                }
            }
            kv_free(s);
        }
    }
    assert_eq!(LIVE.with(|l| l.get()), 0, "stores leaked");
}

#[test]
fn stores_are_independent_and_freed() {
    unsafe {
        let a = kv_new();
        let b = kv_new();
        assert_eq!(LIVE.with(|l| l.get()), 2, "create stores with Store::new, once each");
        kv_add(a, b"x".as_ptr(), 1, 1, std::ptr::null_mut());
        let mut v = 0;
        assert_eq!(kv_get(b, b"x".as_ptr(), 1, &mut v), NOT_FOUND);
        kv_free(a);
        kv_free(b);
        assert_eq!(LIVE.with(|l| l.get()), 0);
    }
}
