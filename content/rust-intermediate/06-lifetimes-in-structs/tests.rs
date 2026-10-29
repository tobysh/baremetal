#[test]
fn tokens_basic() {
    let v: Vec<&str> = Tokens::new("to be or not").collect();
    assert_eq!(v, vec!["to", "be", "or", "not"], "four words");
}

#[test]
fn tokens_irregular_whitespace() {
    let v: Vec<&str> = Tokens::new("  a  bb\tccc\n dddd  ").collect();
    assert_eq!(v, vec!["a", "bb", "ccc", "dddd"], "tabs, newlines and runs of spaces");
}

#[test]
fn tokens_empty_inputs() {
    assert_eq!(Tokens::new("").count(), 0, "empty text");
    assert_eq!(Tokens::new("  \n\t ").count(), 0, "only whitespace");
    let mut t = Tokens::new("one");
    assert_eq!(t.next(), Some("one"), "the only word");
    assert_eq!(t.next(), None, "then nothing");
    assert_eq!(t.next(), None, "and still nothing");
}

#[test]
fn tokens_outlive_the_iterator() {
    let text = String::from("alpha beta gamma");
    let words: Vec<&str>;
    {
        let t = Tokens::new(&text);
        words = t.collect();
    }
    assert_eq!(words, vec!["alpha", "beta", "gamma"], "the items borrow from the text, not from the iterator");
}

#[test]
fn tokens_are_slices_of_the_input() {
    let text = "xx yy";
    let second = Tokens::new(text).nth(1).unwrap();
    assert_eq!(second.as_ptr(), text[3..].as_ptr(), "no copying: the word points into the text");
}

#[test]
fn parse_record_cases() {
    assert_eq!(parse_record("name = Ada"), Some(Record { key: "name", value: "Ada" }), "spaces around =");
    assert_eq!(parse_record("k=v"), Some(Record { key: "k", value: "v" }), "no spaces");
    assert_eq!(parse_record("eq = a = b"), Some(Record { key: "eq", value: "a = b" }), "split at the first =");
    assert_eq!(parse_record("empty ="), Some(Record { key: "empty", value: "" }), "empty value is fine");
    assert_eq!(parse_record("no equals sign"), None, "no =");
    assert_eq!(parse_record(" = value"), None, "empty key");
}

#[test]
fn parse_all_skips_bad_lines() {
    let r = parse_all("a = 1\nbad line\n\nb=22\n=oops\nc = 3");
    let pairs: Vec<(&str, &str)> = r.iter().map(|x| (x.key, x.value)).collect();
    assert_eq!(pairs, vec![("a", "1"), ("b", "22"), ("c", "3")], "valid lines only, in order");
    assert!(parse_all("").is_empty(), "empty text");
}

#[test]
fn longest_value_cases() {
    let text = String::from("a=1\nb=222\nc=33\nd=444");
    let recs = parse_all(&text);
    assert_eq!(longest_value(&recs), Some("222"), "first of the longest values");
    assert_eq!(longest_value(&[]), None, "no records");
}

#[test]
fn longest_value_outlives_the_record_list() {
    let text = String::from("x=short\ny=much longer\nz=mid");
    let v;
    {
        let recs = parse_all(&text);
        v = longest_value(&recs);
    }
    assert_eq!(v, Some("much longer"), "the value borrows from the text, not from the Vec");
}
