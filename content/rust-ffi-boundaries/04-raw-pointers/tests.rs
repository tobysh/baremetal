fn lcg(s: &mut u64) -> usize {
    *s = s.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
    (*s >> 33) as usize
}

fn data(n: usize) -> Vec<i32> {
    (0..n as i32).map(|i| i * 7 - 20).collect()
}

#[test]
fn null_means_empty() {
    unsafe {
        assert_eq!(sum_raw(std::ptr::null(), 5), 0);
        fill_raw(std::ptr::null_mut(), 5, 1);
        reverse_raw(std::ptr::null_mut(), 5);
        let mut v = [1, 2, 3];
        copy_raw(std::ptr::null_mut(), v.as_ptr(), 3);
        copy_raw(v.as_mut_ptr(), std::ptr::null(), 3);
        assert_eq!(v, [1, 2, 3]);
    }
}

#[test]
fn sum_with_wide_totals() {
    let big = vec![i32::MAX; 4];
    unsafe {
        assert_eq!(sum_raw(big.as_ptr(), 4), 4 * i32::MAX as i64);
        assert_eq!(sum_raw(big.as_ptr(), 0), 0);
        let v = data(30);
        for n in [0usize, 1, 2, 29, 30] {
            let want: i64 = v[..n].iter().map(|&x| x as i64).sum();
            assert_eq!(sum_raw(v.as_ptr(), n), want, "n {n}");
        }
    }
}

#[test]
fn fill_stays_inside_len() {
    let mut v = vec![9; 10];
    unsafe { fill_raw(v.as_mut_ptr().add(2), 5, -1) };
    assert_eq!(v, [9, 9, -1, -1, -1, -1, -1, 9, 9, 9], "only elements 2..7");
}

#[test]
fn reverse_matches_reference() {
    for n in 0..9 {
        let mut v = data(n);
        let mut want = v.clone();
        want.reverse();
        unsafe { reverse_raw(v.as_mut_ptr(), n) };
        assert_eq!(v, want, "n {n}");
    }
    let mut v = data(8);
    unsafe { reverse_raw(v.as_mut_ptr().add(2), 4) };
    assert_eq!(&v[..2], &data(8)[..2]);
    assert_eq!(&v[6..], &data(8)[6..]);
}

#[test]
fn copy_handles_overlap_both_ways() {
    let mut s = 11u64;
    for _ in 0..200 {
        let n = 16;
        let len = lcg(&mut s) % 9;
        let from = lcg(&mut s) % (n - len + 1);
        let to = lcg(&mut s) % (n - len + 1);
        let mut v = data(n);
        let mut want = v.clone();
        want.copy_within(from..from + len, to);
        unsafe {
            let base = v.as_mut_ptr();
            copy_raw(base.add(to), base.add(from) as *const i32, len);
        }
        assert_eq!(v, want, "from {from} to {to} len {len}");
    }
}
