#[test]
fn reverses_a_simple_word() {
    assert_eq!(reverse("abc"), "cba", "reverse(\"abc\") should be \"cba\"");
}

#[test]
fn reverses_an_empty_string() {
    assert_eq!(reverse(""), "", "reverse(\"\") should be \"\"");
}

#[test]
fn reverses_a_sentence_with_spaces() {
    assert_eq!(reverse("hello world"), "dlrow olleh", "reverse(\"hello world\") should be \"dlrow olleh\"");
}

#[test]
fn reverses_accented_characters_correctly() {
    assert_eq!(reverse("héllo"), "olléh", "reverse(\"héllo\") should be \"olléh\"; reverse characters, not bytes");
}

#[test]
fn reverses_multi_byte_symbols() {
    assert_eq!(reverse("a→b"), "b→a", "reverse(\"a→b\") should be \"b→a\"");
}

#[test]
fn first_word_stops_at_the_first_space() {
    assert_eq!(first_word("hello world"), "hello", "first_word(\"hello world\") should be \"hello\"");
}

#[test]
fn first_word_of_a_single_word_is_the_whole_word() {
    assert_eq!(first_word("rust"), "rust", "first_word(\"rust\") should be \"rust\"");
}

#[test]
fn first_word_of_empty_string_is_empty() {
    assert_eq!(first_word(""), "", "first_word(\"\") should be \"\"");
}

#[test]
fn first_word_handles_non_ascii_text() {
    assert_eq!(first_word("héllo wörld"), "héllo", "first_word(\"héllo wörld\") should be \"héllo\"");
}

#[test]
fn first_word_with_several_spaces_uses_the_first_one() {
    assert_eq!(first_word("a b c"), "a", "first_word(\"a b c\") should be \"a\"");
}
