#[test]
fn middle_drops_both_ends() {
    assert_eq!(middle(&[1, 2, 3, 4, 5]), &[2, 3, 4], "middle of five");
    assert_eq!(middle(&[7, 8, 9]), &[8], "middle of three");
    let v = vec![10, 20, 30, 40];
    assert_eq!(middle(&v), &[20, 30], "works on a Vec reference");
    assert_eq!(middle(&v[1..]), &[30], "works on a slice of a slice");
}

#[test]
fn middle_of_short_input_is_empty() {
    assert!(middle(&[]).is_empty(), "empty input");
    assert!(middle(&[1]).is_empty(), "one element");
    assert!(middle(&[1, 2]).is_empty(), "two elements");
}

#[test]
fn last_word_finds_the_final_word() {
    assert_eq!(last_word("the quick fox"), "fox", "plain sentence");
    assert_eq!(last_word("  a  bc  "), "bc", "extra spaces");
    assert_eq!(last_word("single"), "single", "one word");
    assert_eq!(last_word("tab\tand\nnewline"), "newline", "any whitespace separates");
    let owned = String::from("hello big world");
    assert_eq!(last_word(&owned), "world", "accepts &String");
}

#[test]
fn last_word_of_blank_is_empty() {
    assert_eq!(last_word(""), "", "empty string");
    assert_eq!(last_word("    "), "", "only spaces");
}

#[test]
fn split_at_char_splits_once() {
    assert_eq!(split_at_char("key=value", '='), ("key", "value"), "key=value");
    assert_eq!(split_at_char("a=b=c", '='), ("a", "b=c"), "only the first separator counts");
    assert_eq!(split_at_char("=x", '='), ("", "x"), "separator first");
    assert_eq!(split_at_char("x=", '='), ("x", ""), "separator last");
}

#[test]
fn split_at_char_handles_missing_and_unicode() {
    assert_eq!(split_at_char("nothing", ':'), ("nothing", ""), "no separator");
    assert_eq!(split_at_char("", ':'), ("", ""), "empty input");
    assert_eq!(split_at_char("naïve→end", '→'), ("naïve", "end"), "multi-byte separator");
    assert_eq!(split_at_char("é:é", ':'), ("é", "é"), "multi-byte neighbours");
}

#[test]
fn join_dash_joins() {
    assert_eq!(join_dash(&["a", "b", "c"]), "a-b-c", "three parts");
    assert_eq!(join_dash(&["solo"]), "solo", "one part");
    assert_eq!(join_dash(&[]), "", "no parts");
    assert_eq!(join_dash(&["", ""]), "-", "empty parts still get a separator");
    let v = vec!["x", "y"];
    assert_eq!(join_dash(&v), "x-y", "accepts a Vec reference");
}
