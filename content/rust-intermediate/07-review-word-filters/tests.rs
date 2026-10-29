const TEXT: &str = "the cat sat on a mat near the cart";

#[test]
fn prefix_and_minlen_directly() {
    assert!(Prefix("ca").matches("cat"), "cat starts with ca");
    assert!(!Prefix("ca").matches("mat"), "mat does not");
    assert!(Prefix("").matches("anything"), "empty prefix matches all");
    assert!(MinLen(3).matches("cat"), "exactly 3");
    assert!(!MinLen(4).matches("cat"), "3 < 4");
    assert!(MinLen(0).matches(""), "min 0");
}

#[test]
fn select_with_each_matcher() {
    assert_eq!(select(TEXT, &Prefix("ca")), vec!["cat", "cart"], "words starting with ca");
    assert_eq!(select(TEXT, &Prefix("t")), vec!["the", "the"], "words starting with t");
    assert_eq!(select(TEXT, &MinLen(4)), vec!["near", "cart"], "words of 4+ letters");
    assert_eq!(select(TEXT, &MinLen(10)), Vec::<&str>::new(), "nothing that long");
}

#[test]
fn select_with_a_closure() {
    assert_eq!(select(TEXT, &|w: &str| w.ends_with('t')), vec!["cat", "sat", "mat", "cart"], "words ending in t");
    assert_eq!(select("", &|_: &str| true), Vec::<&str>::new(), "empty text");
}

#[test]
fn select_all_combines() {
    let t3 = select_all(TEXT, &[&MinLen(3), &|w: &str| w.ends_with('t')]);
    assert_eq!(t3, vec!["cat", "sat", "mat", "cart"], "3+ letters ending in t");
    let both = select_all(TEXT, &[&Prefix("c"), &MinLen(4)]);
    assert_eq!(both, vec!["cart"], "starts with c and 4+ letters");
    let none = select_all(TEXT, &[&Prefix("c"), &Prefix("m")]);
    assert!(none.is_empty(), "no word starts with both c and m");
}

#[test]
fn select_all_with_no_matchers() {
    assert_eq!(select_all("a bb ccc", &[]).len(), 3, "an empty matcher list matches every word");
}

#[test]
fn first_match_cases() {
    assert_eq!(first_match(TEXT, &Prefix("m")), Some("mat"), "first m word");
    assert_eq!(first_match(TEXT, &MinLen(5)), None, "nothing 5 letters long");
    assert_eq!(first_match("zed alpha", &MinLen(4)), Some("alpha"), "different text");
}

#[test]
fn results_outlive_temporary_matchers() {
    let text = String::from("alpha beta alphabet gamma");
    let found;
    {
        let wanted = String::from("alph");
        let m = Prefix(&wanted);
        found = select(&text, &m);
    }
    assert_eq!(found, vec!["alpha", "alphabet"], "the words borrow from the text, not from the matcher");
}
