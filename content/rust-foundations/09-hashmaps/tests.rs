#[test]
fn counts_repeated_words() {
    let m = word_count("a b a c a b");
    assert_eq!(m.get("a"), Some(&3), "\"a\" should appear 3 times");
    assert_eq!(m.get("b"), Some(&2), "\"b\" should appear 2 times");
    assert_eq!(m.get("c"), Some(&1), "\"c\" should appear 1 time");
    assert_eq!(m.len(), 3, "there should be exactly 3 distinct words");
}

#[test]
fn empty_text_gives_empty_map() {
    assert!(word_count("").is_empty(), "word_count(\"\") should be empty");
    assert!(word_count("   \n\t ").is_empty(), "only whitespace should give an empty map");
}

#[test]
fn ignores_case() {
    let m = word_count("Rust rust RUST");
    assert_eq!(m.get("rust"), Some(&3), "\"Rust rust RUST\" should count \"rust\" 3 times");
    assert_eq!(m.len(), 1, "case variants should be one word");
}

#[test]
fn strips_surrounding_punctuation() {
    let m = word_count("Hello, world! (hello) \"world\".");
    assert_eq!(m.get("hello"), Some(&2), "\"hello\" should be counted twice");
    assert_eq!(m.get("world"), Some(&2), "\"world\" should be counted twice");
    assert_eq!(m.len(), 2, "punctuation must not create extra words");
}

#[test]
fn keeps_punctuation_inside_words() {
    let m = word_count("don't stop");
    assert_eq!(m.get("don't"), Some(&1), "the apostrophe inside \"don't\" must be kept");
}

#[test]
fn skips_words_that_are_only_punctuation() {
    let m = word_count("-- hi ... !!");
    assert_eq!(m.len(), 1, "words made only of punctuation should be skipped");
    assert_eq!(m.get("hi"), Some(&1), "\"hi\" should be counted once");
}

#[test]
fn splits_on_any_whitespace() {
    let m = word_count("one\ntwo\tone   two  one");
    assert_eq!(m.get("one"), Some(&3), "\"one\" should appear 3 times");
    assert_eq!(m.get("two"), Some(&2), "\"two\" should appear 2 times");
}

#[test]
fn full_sentence_example() {
    let m = word_count("The cat; the DOG. the end!");
    assert_eq!(m.get("the"), Some(&3), "\"the\" should appear 3 times");
    assert_eq!(m.get("cat"), Some(&1), "\"cat\" should appear once");
    assert_eq!(m.get("dog"), Some(&1), "\"dog\" should appear once");
    assert_eq!(m.get("end"), Some(&1), "\"end\" should appear once");
    assert_eq!(m.len(), 4, "there should be 4 distinct words");
}
