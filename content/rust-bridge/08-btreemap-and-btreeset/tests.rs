fn table() -> BTreeMap<u32, String> {
    let mut p = BTreeMap::new();
    for (price, name) in [(20, "book"), (5, "pen"), (50, "bag"), (12, "ruler"), (0, "flyer")] {
        p.insert(price, name.to_string());
    }
    p
}

#[test]
fn word_index() {
    let idx = index_words(&["The cat", "the dog the end", "CAT", ""]);
    assert_eq!(idx.get("the"), Some(&vec![1, 2]), "the appears on lines 1 and 2 once each");
    assert_eq!(idx.get("cat"), Some(&vec![1, 3]), "case folds");
    assert_eq!(idx.get("end"), Some(&vec![2]), "end");
    let keys: Vec<&str> = idx.keys().map(|k| k.as_str()).collect();
    assert_eq!(keys, vec!["cat", "dog", "end", "the"], "keys are sorted");
    assert!(index_words(&[]).is_empty(), "no lines");
}

#[test]
fn repeated_words() {
    let idx = index_words(&["b a", "a", "c", "a b"]);
    assert_eq!(alphabetical(&idx), vec!["a".to_string(), "b".to_string()], "a and b repeat");
    assert_eq!(idx.get("a"), Some(&vec![1, 2, 4]), "a on three lines");
    assert!(alphabetical(&index_words(&["x y", "z"])).is_empty(), "nothing repeats");
}

#[test]
fn price_ranges() {
    let p = table();
    assert_eq!(between(&p, 5, 20), vec!["pen", "ruler", "book"], "inclusive both ends, price order");
    assert_eq!(between(&p, 6, 11), Vec::<String>::new(), "nothing in the gap");
    assert_eq!(between(&p, 0, 0), vec!["flyer"], "single price");
    assert_eq!(between(&p, 30, 10), Vec::<String>::new(), "reversed range");
    assert_eq!(between(&p, 0, u32::MAX).len(), 5, "everything");
}

#[test]
fn ceilings_and_floors() {
    let p = table();
    assert_eq!(cheapest_at_least(&p, 6), Some(&"ruler".to_string()), "6 -> 12");
    assert_eq!(cheapest_at_least(&p, 12), Some(&"ruler".to_string()), "exact match counts");
    assert_eq!(cheapest_at_least(&p, 51), None, "nothing that dear");
    assert_eq!(priciest_below(&p, 50), Some(&"book".to_string()), "below 50 -> 20");
    assert_eq!(priciest_below(&p, 6), Some(&"pen".to_string()), "below 6 -> 5");
    assert_eq!(priciest_below(&p, 0), None, "nothing below 0");
    assert_eq!(priciest_below(&BTreeMap::new(), 9), None, "empty table");
}

#[test]
fn gaps() {
    assert_eq!(distinct_gaps(&[1, 4, 4, 7, 8]), BTreeSet::from([1, 3]), "gaps 3, 3, 1");
    assert_eq!(distinct_gaps(&[5]), BTreeSet::new(), "single");
    assert_eq!(distinct_gaps(&[2, 2, 2]), BTreeSet::new(), "one distinct value");
    assert_eq!(distinct_gaps(&[10, 0, 5]), BTreeSet::from([5]), "unsorted input");
    assert_eq!(distinct_gaps(&[-3, 3]), BTreeSet::from([6]), "across zero");
}
