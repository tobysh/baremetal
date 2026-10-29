#[test]
fn length_scores() {
    assert_eq!(LengthScorer.score("horse"), 5, "length of horse");
    assert_eq!(LengthScorer.score(""), 0, "length of the empty word");
    assert_eq!(LengthScorer.total(&["a", "abc"]), 4, "total length");
    assert_eq!(LengthScorer.total(&["hello", "to", "everyone", "here"]), 19, "total length of four words");
}

#[test]
fn vowel_scores() {
    assert_eq!(VowelScorer.score("education"), 5, "education has five vowels");
    assert_eq!(VowelScorer.score("rhythm"), 0, "rhythm has none");
    assert_eq!(VowelScorer.score("AEIoux"), 5, "upper case counts");
    assert_eq!(VowelScorer.total(&["tree", "sky", "idea"]), 5, "total vowels");
}

#[test]
fn weighted_scores() {
    assert_eq!(WeightedScorer { weight: 3 }.score("abcd"), 12, "4 letters times 3");
    assert_eq!(WeightedScorer { weight: 0 }.score("abcd"), 0, "weight 0");
    assert_eq!(WeightedScorer { weight: 2 }.total(&["ab", "c"]), 6, "(2 + 1) * 2");
}

#[test]
fn best_prefers_first_on_tie() {
    assert_eq!(LengthScorer.best(&["cat", "horse", "dog", "mouse"]), Some(1), "horse and mouse tie, the first wins");
    assert_eq!(LengthScorer.best(&["aaaa", "b"]), Some(0), "longest first");
    assert_eq!(LengthScorer.best(&["b", "aaaa"]), Some(1), "longest last");
}

#[test]
fn best_with_other_scorers() {
    assert_eq!(VowelScorer.best(&["sky", "tree", "aeiou"]), Some(2), "most vowels");
    assert_eq!(VowelScorer.best(&["sky", "rhythm"]), Some(0), "all zero: the first");
    assert_eq!(WeightedScorer { weight: 7 }.best(&["xy", "xyz", "x"]), Some(1), "weighted best");
}

#[test]
fn best_of_nothing() {
    assert_eq!(LengthScorer.best(&[]), None, "an empty slice has no best word");
    assert_eq!(VowelScorer.total(&[]), 0, "an empty slice totals 0");
}

#[test]
fn labels_use_the_constant() {
    assert_eq!(LengthScorer.label(), "scorer:length", "length label");
    assert_eq!(VowelScorer.label(), "scorer:vowel", "vowel label");
    assert_eq!(WeightedScorer { weight: 1 }.label(), "scorer:weighted", "weighted label");
    assert_eq!(<VowelScorer as Scorer>::NAME, "vowel", "the constant itself");
}
