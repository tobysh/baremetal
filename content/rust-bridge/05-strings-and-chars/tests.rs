#[test]
fn palindromes() {
    assert!(is_palindrome("A man, a plan, a canal: Panama"), "classic");
    assert!(is_palindrome("racecar"), "plain");
    assert!(is_palindrome(""), "empty");
    assert!(is_palindrome("?!"), "punctuation only");
    assert!(is_palindrome("Été"), "accents are letters, case folds");
    assert!(!is_palindrome("hello"), "not one");
    assert!(!is_palindrome("ab"), "two different");
}

#[test]
fn title_cases() {
    assert_eq!(title_case("  hELLO   wORLD "), "Hello World", "spacing and case");
    assert_eq!(title_case("a"), "A", "single letter");
    assert_eq!(title_case(""), "", "empty");
    assert_eq!(title_case("élan vital"), "Élan Vital", "non-ASCII first letter");
    assert_eq!(title_case("one two three"), "One Two Three", "three words");
}

#[test]
fn truncation_counts_characters() {
    assert_eq!(truncate_chars("hello", 3), "hel", "ascii");
    assert_eq!(truncate_chars("héllo wörld", 4), "héll", "accent inside the cut");
    assert_eq!(truncate_chars("ab", 10), "ab", "shorter than max");
    assert_eq!(truncate_chars("ab", 0), "", "zero");
    assert_eq!(truncate_chars("😀😀😀", 2), "😀😀", "emoji are one character each");
}

#[test]
fn caesar_shifts() {
    assert_eq!(caesar("Hello, World!", 3), "Khoor, Zruog!", "shift 3");
    assert_eq!(caesar("xyz", 3), "abc", "wraps around");
    assert_eq!(caesar("abc", -1), "zab", "negative");
    assert_eq!(caesar("Abc", 26), "Abc", "full turn");
    assert_eq!(caesar("Abc", 53), "Bcd", "more than two turns");
    assert_eq!(caesar("Abc", -53), "Zab", "negative beyond two turns");
    assert_eq!(caesar("héllo 1", 1), "iémmp 1", "non-ASCII and digits untouched");
}

#[test]
fn key_values() {
    assert_eq!(split_key_value("name=Ada"), Some(("name", "Ada")), "plain");
    assert_eq!(split_key_value("  k  =  v v  "), Some(("k", "v v")), "trimmed, inner space kept");
    assert_eq!(split_key_value("a=b=c"), Some(("a", "b=c")), "first = only");
    assert_eq!(split_key_value("empty="), Some(("empty", "")), "empty value");
    assert_eq!(split_key_value("=x"), None, "empty key");
    assert_eq!(split_key_value("   =x"), None, "blank key");
    assert_eq!(split_key_value("novalue"), None, "no equals sign");
}
