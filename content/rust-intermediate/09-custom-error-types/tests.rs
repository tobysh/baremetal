fn pairs(text: &str) -> Vec<(String, i64)> {
    parse_config(text).expect("this config is valid")
}

#[test]
fn parses_settings_in_order() {
    let v = pairs("width = 80\nheight=24\ndepth = -3");
    assert_eq!(v, vec![("width".to_string(), 80), ("height".to_string(), 24), ("depth".to_string(), -3)], "three settings");
}

#[test]
fn skips_comments_and_blanks() {
    let v = pairs("# header\n\n  a = 1  \n   # indented comment\nb=2\n");
    assert_eq!(v, vec![("a".to_string(), 1), ("b".to_string(), 2)], "comments and blank lines are ignored");
}

#[test]
fn missing_equals_counts_every_line() {
    assert_eq!(parse_config("a = 1\n# c\n\nbroken line"), Err(ConfigError::MissingEquals(4)), "line 4 counting skipped lines");
    assert_eq!(parse_config("oops"), Err(ConfigError::MissingEquals(1)), "line 1");
}

#[test]
fn bad_numbers() {
    assert_eq!(parse_config("a = 1\nb = x1"), Err(ConfigError::BadNumber { line: 2, text: "x1".to_string() }), "x1 on line 2");
    assert_eq!(parse_config("c =   "), Err(ConfigError::BadNumber { line: 1, text: String::new() }), "empty value");
    assert_eq!(parse_config("n = 1.5"), Err(ConfigError::BadNumber { line: 1, text: "1.5".to_string() }), "not an integer");
}

#[test]
fn duplicates() {
    assert_eq!(parse_config("a = 1\nb = 2\na = 3"), Err(ConfigError::Duplicate("a".to_string())), "a repeats");
    assert_eq!(parse_config("a = 1\na = x"), Err(ConfigError::BadNumber { line: 2, text: "x".to_string() }), "the number is checked before the duplicate");
}

#[test]
fn first_error_wins() {
    assert_eq!(parse_config("a = z\nbroken"), Err(ConfigError::BadNumber { line: 1, text: "z".to_string() }), "earlier line first");
}

#[test]
fn nothing_to_parse() {
    assert_eq!(parse_config(""), Err(ConfigError::NoSettings), "empty text");
    assert_eq!(parse_config("# only a comment\n\n"), Err(ConfigError::NoSettings), "only comments");
}

#[test]
fn messages() {
    assert_eq!(ConfigError::NoSettings.to_string(), "no settings found", "NoSettings");
    assert_eq!(ConfigError::MissingEquals(3).to_string(), "line 3: expected key = value", "MissingEquals");
    assert_eq!(ConfigError::BadNumber { line: 2, text: "x1".to_string() }.to_string(), "line 2: 'x1' is not a number", "BadNumber");
    assert_eq!(ConfigError::Duplicate("width".to_string()).to_string(), "duplicate key 'width'", "Duplicate");
    assert_eq!(ConfigError::MissingEquals(12).to_string(), "line 12: expected key = value", "another line number");
}

#[test]
fn works_as_a_boxed_error() {
    let e: Box<dyn Error> = Box::new(ConfigError::Duplicate("k".to_string()));
    assert_eq!(e.to_string(), "duplicate key 'k'", "Display survives boxing");
}
