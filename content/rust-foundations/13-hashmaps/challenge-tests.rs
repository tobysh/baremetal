fn pair(w: &str, n: usize) -> (String, usize) {
    (w.to_string(), n)
}

#[test]
fn highest_count_first() {
    let got = top_words("the cat and the hat and the bat", 2);
    assert_eq!(got, vec![pair("the", 3), pair("and", 2)], "the is 3, and is 2");
}

#[test]
fn ties_are_alphabetical() {
    let got = top_words("pear apple fig apple pear fig kiwi", 4);
    assert_eq!(
        got,
        vec![pair("apple", 2), pair("fig", 2), pair("pear", 2), pair("kiwi", 1)],
        "equal counts are ordered a to z"
    );
}

#[test]
fn counts_words_like_word_count() {
    let got = top_words("Don't stop! don't STOP... Stop, stop -- go.", 3);
    assert_eq!(got, vec![pair("stop", 4), pair("don't", 2), pair("go", 1)], "lowercase, strip punctuation at the ends, keep inner apostrophes");
}

#[test]
fn fewer_words_than_n() {
    let got = top_words("b a b", 10);
    assert_eq!(got, vec![pair("b", 2), pair("a", 1)], "return everything when n is large");
}

#[test]
fn zero_and_empty() {
    assert!(top_words("some words here", 0).is_empty(), "n = 0 gives nothing");
    assert!(top_words("", 3).is_empty(), "empty text gives nothing");
    assert!(top_words("-- ... !!", 3).is_empty(), "punctuation-only words are skipped");
}

#[test]
fn result_is_stable_for_a_long_text() {
    let mut text = String::new();
    for i in 0..300 {
        let w = format!("w{}", i % 17);
        for _ in 0..(1 + i % 5) {
            text.push_str(&w);
            text.push(' ');
        }
    }
    let a = top_words(&text, 17);
    let b = top_words(&text, 17);
    assert_eq!(a, b, "the same input must always give the same answer");
    assert_eq!(a.len(), 17, "17 distinct words");
    for w in a.windows(2) {
        assert!(w[0].1 > w[1].1 || (w[0].1 == w[1].1 && w[0].0 < w[1].0), "order broken between {:?} and {:?}", w[0], w[1]);
    }
    let total: usize = a.iter().map(|p| p.1).sum();
    assert_eq!(total, text.split_whitespace().count(), "counts must add up to the number of words");
}
