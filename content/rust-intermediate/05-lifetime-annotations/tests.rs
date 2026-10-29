#[test]
fn longest_picks_longer() {
    assert_eq!(longest("apple", "fig"), "apple", "first is longer");
    assert_eq!(longest("fig", "banana"), "banana", "second is longer");
    assert_eq!(longest("", "x"), "x", "empty versus one letter");
}

#[test]
fn longest_tie_returns_first() {
    let a = String::from("abc");
    let b = String::from("xyz");
    assert_eq!(longest(&a, &b).as_ptr(), a.as_ptr(), "on a tie the result must borrow from the first argument");
}

#[test]
fn first_word_cases() {
    assert_eq!(first_word("hello world"), "hello", "two words");
    assert_eq!(first_word("   padded  text"), "padded", "leading spaces");
    assert_eq!(first_word("single"), "single", "one word");
    assert_eq!(first_word(""), "", "empty");
    assert_eq!(first_word(" \t\n "), "", "only whitespace");
}

#[test]
fn split_pair_cases() {
    assert_eq!(split_pair("key=value", '='), Some(("key", "value")), "simple");
    assert_eq!(split_pair("a=b=c", '='), Some(("a", "b=c")), "only the first separator counts");
    assert_eq!(split_pair("novalue", '='), None, "no separator");
    assert_eq!(split_pair("k=", '='), Some(("k", "")), "empty after");
    assert_eq!(split_pair("=v", '='), Some(("", "v")), "empty before");
    assert_eq!(split_pair("12:30:45", ':'), Some(("12", "30:45")), "colon");
}

#[test]
fn split_pair_multibyte_separator() {
    assert_eq!(split_pair("a\u{e9}bc", '\u{e9}'), Some(("a", "bc")), "the separator is two bytes long");
    assert_eq!(split_pair("\u{2192}x", '\u{2192}'), Some(("", "x")), "a three-byte separator");
}

#[test]
fn strip_known_cases() {
    assert_eq!(strip_known("v2:alpha", "v2:"), "alpha", "prefix present");
    assert_eq!(strip_known("v2:alpha", "v3:"), "v2:alpha", "prefix absent");
    assert_eq!(strip_known("abc", ""), "abc", "empty prefix");
    assert_eq!(strip_known("abc", "abc"), "", "the whole string");
}

#[test]
fn strip_known_result_outlives_the_prefix() {
    let text = String::from("v2:alpha");
    let out;
    {
        let prefix = String::from("v2:");
        out = strip_known(&text, &prefix);
    }
    assert_eq!(out, "alpha", "the result borrows only from text, so dropping prefix is fine");
}

#[test]
fn longest_line_cases() {
    assert_eq!(longest_line("ab\nabcd\nxyz"), "abcd", "middle line");
    assert_eq!(longest_line(""), "", "empty text");
    assert_eq!(longest_line("one"), "one", "one line");
    let text = String::from("aa\nbb\ncc");
    assert_eq!(longest_line(&text).as_ptr(), text.as_ptr(), "the first line wins a tie, and it borrows from the text");
}
