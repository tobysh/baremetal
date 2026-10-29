#[test]
fn duplicates() {
    assert_eq!(first_duplicate(&[1, 2, 3, 2, 1]), Some(2), "2 repeats first");
    assert_eq!(first_duplicate(&[5, 5]), Some(5), "adjacent");
    assert_eq!(first_duplicate(&[1, 2, 3]), None, "none");
    assert_eq!(first_duplicate(&[]), None, "empty");
    assert_eq!(first_duplicate(&[9, 8, 7, 8, 9, 7]), Some(8), "scanning order decides");
}

#[test]
fn counting_words() {
    let c = word_counts("the cat The dog  the");
    assert_eq!(c.get("the"), Some(&3), "case-insensitive");
    assert_eq!(c.get("cat"), Some(&1), "cat");
    assert_eq!(c.len(), 3, "three distinct words");
    assert!(word_counts("  ").is_empty(), "no words");
}

#[test]
fn grouping() {
    let g = group_by_first_letter(&["apple", "Avocado", "bean", "", "axe", "Bell"]);
    assert_eq!(g.get(&'a'), Some(&vec!["apple".to_string(), "Avocado".to_string(), "axe".to_string()]), "a-words in order");
    assert_eq!(g.get(&'b'), Some(&vec!["bean".to_string(), "Bell".to_string()]), "b-words");
    assert_eq!(g.len(), 2, "empty word is skipped");
}

#[test]
fn intersections() {
    assert_eq!(common_sorted(&[1, 2, 3, 3], &[3, 2, 9]), vec![2, 3], "overlap once each");
    assert_eq!(common_sorted(&[1], &[2]), Vec::<i32>::new(), "disjoint");
    assert_eq!(common_sorted(&[-1, 5, 5, 0], &[0, -1, -1]), vec![-1, 0], "negatives and zero");
    assert_eq!(common_sorted(&[], &[1]), Vec::<i32>::new(), "empty");
}

#[test]
fn differences() {
    assert_eq!(only_in_first_sorted(&[1, 2, 3, 3], &[3, 2, 9]), vec![1], "one left");
    assert_eq!(only_in_first_sorted(&[4, 4, 2], &[]), vec![2, 4], "nothing to remove");
    assert_eq!(only_in_first_sorted(&[1, 2], &[2, 1]), Vec::<i32>::new(), "all removed");
}

#[test]
fn most_common_word() {
    assert_eq!(most_common("a b b a c"), Some(("a".to_string(), 2)), "tie goes to the smaller word");
    assert_eq!(most_common("x Y y z"), Some(("y".to_string(), 2)), "case folds");
    assert_eq!(most_common("solo"), Some(("solo".to_string(), 1)), "single word");
    assert_eq!(most_common(""), None, "no words");
}
