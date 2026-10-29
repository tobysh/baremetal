fn lines(v: &[&str]) -> Vec<String> { v.iter().map(|s| s.to_string()).collect() }

#[test]
fn counts_words_case_insensitively() {
    let (m, total) = census(lines(&["the cat", "The dog and the cat"]), 2);
    assert_eq!(total, 7, "7 words in total");
    assert_eq!(m.get("the"), Some(&3), "'the' and 'The' are the same word");
    assert_eq!(m.get("cat"), Some(&2));
    assert_eq!(m.get("dog"), Some(&1));
    assert_eq!(m.len(), 4, "the, cat, dog, and");
}

#[test]
fn result_does_not_depend_on_worker_count() {
    let input = lines(&["a b c", "b c d", "c d e", "", "  a   a  ", "E e e"]);
    let base = census(input.clone(), 1);
    for w in [2, 3, 6, 10] {
        assert_eq!(census(input.clone(), w), base, "with {w} workers the answer must be identical");
    }
    assert_eq!(base.1, 14, "14 words overall");
    assert_eq!(base.0.get("e"), Some(&4));
}

#[test]
fn zero_workers_means_one() {
    let (m, total) = census(lines(&["x y x"]), 0);
    assert_eq!(total, 3);
    assert_eq!(m.get("x"), Some(&2), "workers == 0 should behave like 1");
}

#[test]
fn no_lines() {
    let (m, total) = census(Vec::new(), 4);
    assert!(m.is_empty(), "no lines, no words");
    assert_eq!(total, 0);
}

#[test]
fn large_input() {
    let input: Vec<String> = (0..400).map(|i| format!("alpha beta w{}", i % 10)).collect();
    let (m, total) = census(input, 5);
    assert_eq!(total, 1200, "400 lines x 3 words");
    assert_eq!(m.get("alpha"), Some(&400));
    assert_eq!(m.get("w7"), Some(&40), "each of the 10 numbered words appears 40 times");
}

#[test]
fn keys_come_back_sorted() {
    let (m, _) = census(lines(&["pear apple mango", "zebra"]), 3);
    let keys: Vec<&String> = m.keys().collect();
    assert_eq!(keys, vec!["apple", "mango", "pear", "zebra"], "BTreeMap keeps keys sorted");
}
