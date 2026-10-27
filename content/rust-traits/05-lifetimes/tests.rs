#[test]
fn longest_picks_longer() {
    assert_eq!(longest("tower", "keep"), "tower", "first is longer");
    assert_eq!(longest("ox", "castle"), "castle", "second is longer");
}

#[test]
fn longest_tie_returns_first() {
    let a = "abc";
    let b = "xyz";
    assert!(std::ptr::eq(longest(a, b), a), "on a tie the first argument should be returned");
}

#[test]
fn longest_with_different_scopes() {
    let outer = String::from("long string is long");
    let result;
    {
        let inner = String::from("short");
        result = longest(outer.as_str(), inner.as_str()).len();
    }
    assert_eq!(result, 19, "length of the longer string");
}

#[test]
fn first_word_basic() {
    assert_eq!(first_word("hello world"), "hello", "first word of two");
    assert_eq!(first_word("single"), "single", "no space returns everything");
    assert_eq!(first_word(""), "", "empty string");
}

#[test]
fn words_iterates_all() {
    let mut w = Words::new("one two  three");
    assert_eq!(w.next_word(), Some("one"), "first word");
    assert_eq!(w.next_word(), Some("two"), "second word");
    assert_eq!(w.next_word(), Some("three"), "third word skips double space");
    assert_eq!(w.next_word(), None, "then None");
    assert_eq!(w.next_word(), None, "and None again");
}

#[test]
fn words_handles_leading_and_trailing_spaces() {
    let mut w = Words::new("   padded   ");
    assert_eq!(w.next_word(), Some("padded"), "word between spaces");
    assert_eq!(w.next_word(), None, "only spaces remain");
}

#[test]
fn words_empty_input() {
    let mut w = Words::new("");
    assert_eq!(w.next_word(), None, "empty input has no words");
}

#[test]
fn earlier_words_outlive_later_calls() {
    let text = String::from("alpha beta");
    let mut w = Words::new(&text);
    let a = w.next_word().unwrap();
    let b = w.next_word().unwrap();
    assert_eq!((a, b), ("alpha", "beta"), "words must stay usable after advancing the parser");
}

#[test]
fn words_returns_slices_of_the_input() {
    let text = "keep it zero copy";
    let mut w = Words::new(text);
    let first = w.next_word().unwrap();
    assert_eq!(first.as_ptr(), text.as_ptr(), "word should point into the original text, not a copy");
}
