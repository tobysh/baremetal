#[test]
fn sum_of_squares() {
    let nums: Vec<u64> = (1..=1000).collect();
    let expected: u64 = nums.iter().map(|n| n * n).sum();
    for w in [0, 1, 2, 3, 7, 12] {
        assert_eq!(map_reduce(&nums, w, |n| n * n, 0u64, |a, b| a + b), expected, "workers = {w}");
    }
}

#[test]
fn empty_input_gives_identity() {
    let nothing: Vec<i32> = Vec::new();
    assert_eq!(map_reduce(&nothing, 4, |n| *n, 99, |a, b| a + b), 99, "no items: the identity comes back");
}

#[test]
fn order_is_preserved_for_non_commutative_reduce() {
    let items: Vec<u32> = (0..50).collect();
    let expected: String = items.iter().map(|i| format!("{i},")).collect();
    for w in [1, 4, 6, 13] {
        let got = map_reduce(&items, w, |i| format!("{i},"), String::new(), |a, b| a + &b);
        assert_eq!(got, expected, "string concatenation must keep input order with {w} workers");
    }
}

#[test]
fn works_with_max_and_borrowed_data() {
    let words = vec!["pear", "fig", "banana", "kiwi"];
    let limit = 10; // borrowed by the closure, so no 'static requirement
    let longest = map_reduce(&words, 2, |w| w.len().min(limit), 0usize, |a, b| a.max(b));
    assert_eq!(longest, 6, "the longest word is banana");
}

#[test]
fn map_runs_on_other_threads() {
    let items: Vec<u32> = (0..8).collect();
    let ids = map_reduce(
        &items,
        4,
        |_| vec![std::thread::current().id()],
        Vec::new(),
        |mut a, b| { a.extend(b); a },
    );
    assert_eq!(ids.len(), 8);
    assert!(ids.iter().all(|id| *id != std::thread::current().id()), "map must not run on the calling thread");
    let distinct: std::collections::HashSet<_> = ids.iter().collect();
    assert!(distinct.len() >= 2, "work should be spread over several threads, saw {}", distinct.len());
}

#[test]
fn word_frequencies_basic() {
    let m = word_frequencies(&["the cat", "The dog and the cat", ""], 2);
    assert_eq!(m.get("the"), Some(&3));
    assert_eq!(m.get("cat"), Some(&2));
    assert_eq!(m.len(), 4);
}

#[test]
fn word_frequencies_independent_of_workers() {
    let texts: Vec<String> = (0..60).map(|i| format!("a b{} C c", i % 5)).collect();
    let refs: Vec<&str> = texts.iter().map(|s| s.as_str()).collect();
    let base = word_frequencies(&refs, 1);
    assert_eq!(base.get("c"), Some(&120), "C and c both count as 'c'");
    assert_eq!(base.get("b3"), Some(&12));
    for w in [2, 5, 11] {
        assert_eq!(word_frequencies(&refs, w), base, "same answer with {w} workers");
    }
    assert!(word_frequencies(&[], 3).is_empty());
}

#[test]
fn mapped_values_need_send_but_not_sync() {
    // Cell is Send and Clone, but not Sync. Each seed/result is owned by one thread.
    let total = map_reduce(
        &[2, 5, 8, 11], 3,
        |&n| std::cell::Cell::new(n),
        std::cell::Cell::new(0),
        |a, b| std::cell::Cell::new(a.into_inner() + b.into_inner()),
    );
    assert_eq!(total.into_inner(), 26);
}
