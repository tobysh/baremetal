extern "C" fn t_sub(a: i32, b: i32) -> i32 {
    a.wrapping_sub(b)
}

extern "C" fn t_second(_a: i32, b: i32) -> i32 {
    b
}

fn lcg(s: &mut u64) -> i32 {
    *s = s.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
    (*s >> 32) as i32
}

#[test]
fn operations_on_edge_values() {
    let add = lookup("add").expect("add");
    let mul = lookup("mul").expect("mul");
    let max = lookup("max").expect("max");
    assert_eq!(add(2, 3), 5);
    assert_eq!(add(i32::MAX, 1), i32::MIN, "wraps, does not panic");
    assert_eq!(mul(-4, 6), -24);
    assert_eq!(mul(65536, 65536), 0, "wraps to zero");
    assert_eq!(mul(i32::MIN, -1), i32::MIN);
    assert_eq!(max(-3, -9), -3);
    assert_eq!(max(7, 7), 7);
    assert_eq!(max(i32::MIN, i32::MAX), i32::MAX);
}

#[test]
fn lookup_is_exact() {
    for bad in ["", "ADD", "Add", "add ", "sub", "maximum", "mu"] {
        assert!(lookup(bad).is_none(), "{bad:?} should not be found");
    }
}

#[test]
fn names_come_from_the_table() {
    assert_eq!(names(), vec!["add", "max", "mul"]);
    for n in names() {
        assert!(lookup(n).is_some(), "{n} listed but not found");
    }
}

#[test]
fn fold_matches_a_reference_for_every_operation() {
    let mut s = 5u64;
    for len in [0usize, 1, 2, 3, 10, 40] {
        let xs: Vec<i32> = (0..len).map(|_| lcg(&mut s)).collect();
        let init = lcg(&mut s);
        let add = xs.iter().fold(init, |a, &x| a.wrapping_add(x));
        let mul = xs.iter().fold(init, |a, &x| a.wrapping_mul(x));
        let max = xs.iter().fold(init, |a, &x| a.max(x));
        assert_eq!(fold_with(lookup("add").unwrap(), init, &xs), add, "add len {len}");
        assert_eq!(fold_with(lookup("mul").unwrap(), init, &xs), mul, "mul len {len}");
        assert_eq!(fold_with(lookup("max").unwrap(), init, &xs), max, "max len {len}");
    }
}

#[test]
fn fold_works_with_a_caller_supplied_function() {
    // order matters for a non-commutative operation
    assert_eq!(fold_with(t_sub, 100, &[1, 2, 3]), 94);
    assert_eq!(fold_with(t_sub, 0, &[]), 0);
    assert_eq!(fold_with(t_second, -1, &[4, 5, 6]), 6, "last element wins");
    assert_eq!(fold_with(t_second, -1, &[]), -1);
}
