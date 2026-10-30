#[test]
fn shared_prefix_of_several_words() {
    assert_eq!(common_prefix(&["flower", "flow", "flight"]), "fl", "fl");
    assert_eq!(common_prefix(&["interview", "internet", "interval"]), "inter", "inter");
    assert_eq!(common_prefix(&["abc", "abc", "abc"]), "abc", "identical words");
}

#[test]
fn no_shared_prefix() {
    assert_eq!(common_prefix(&["dog", "racecar", "car"]), "", "nothing shared");
    assert_eq!(common_prefix(&["Cat", "cat"]), "", "case-sensitive");
}

#[test]
fn edge_cases() {
    assert_eq!(common_prefix(&[]), "", "no words");
    assert_eq!(common_prefix(&["solo"]), "solo", "one word is its own prefix");
    assert_eq!(common_prefix(&["", "abc"]), "", "empty word first");
    assert_eq!(common_prefix(&["abc", ""]), "", "empty word last");
    assert_eq!(common_prefix(&["ab", "abcd", "abc"]), "ab", "the shortest word limits it");
}

#[test]
fn works_on_characters_not_bytes() {
    assert_eq!(common_prefix(&["naïve", "naïf"]), "naï", "multi-byte char is shared");
    assert_eq!(common_prefix(&["éa", "éb"]), "é", "prefix ends after a 2-byte char");
    assert_eq!(common_prefix(&["é", "è"]), "", "same first byte, different chars");
    assert_eq!(common_prefix(&["→x", "→y", "→"]), "→", "3-byte char");
}

#[test]
fn result_borrows_from_the_first_word() {
    let owned = String::from("prefix-one");
    let other = String::from("prefix-two");
    let got = common_prefix(&[owned.as_str(), other.as_str()]);
    assert_eq!(got, "prefix-", "prefix with dash");
    assert_eq!(got.as_ptr(), owned.as_ptr(), "must be a slice of the first word, not a copy");
}
