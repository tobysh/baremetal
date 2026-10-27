fn num(n: i64) -> Token {
    Token::Num(n)
}
fn id(s: &str) -> Token {
    Token::Ident(s.to_string())
}

#[test]
fn empty_and_blank() {
    assert_eq!(tokenize(""), Ok(vec![]), "empty input");
    assert_eq!(tokenize("  \t\n "), Ok(vec![]), "only whitespace");
}

#[test]
fn single_symbols() {
    assert_eq!(
        tokenize("+-*/()"),
        Ok(vec![Token::Plus, Token::Minus, Token::Star, Token::Slash, Token::LParen, Token::RParen]),
        "each symbol is its own token"
    );
}

#[test]
fn numbers() {
    assert_eq!(tokenize("42"), Ok(vec![num(42)]), "a number at the end of the input");
    assert_eq!(tokenize("007"), Ok(vec![num(7)]), "leading zeros");
    assert_eq!(tokenize("1 22 333"), Ok(vec![num(1), num(22), num(333)]), "several numbers");
}

#[test]
fn names() {
    assert_eq!(tokenize("x"), Ok(vec![id("x")]), "single letter at the end");
    assert_eq!(tokenize("_tmp9 count"), Ok(vec![id("_tmp9"), id("count")]), "underscores and digits");
    assert_eq!(tokenize("café"), Ok(vec![id("café")]), "unicode letters");
}

#[test]
fn a_full_expression() {
    assert_eq!(
        tokenize("12 + count * (3 - x)"),
        Ok(vec![
            num(12), Token::Plus, id("count"), Token::Star, Token::LParen,
            num(3), Token::Minus, id("x"), Token::RParen,
        ]),
        "a mixed expression"
    );
}

#[test]
fn touching_tokens() {
    assert_eq!(tokenize("12ab"), Ok(vec![num(12), id("ab")]), "a number then a name");
    assert_eq!(tokenize("a1"), Ok(vec![id("a1")]), "digits continue a name");
    assert_eq!(tokenize("3*(4)"), Ok(vec![num(3), Token::Star, Token::LParen, num(4), Token::RParen]), "no spaces needed");
}

#[test]
fn minus_is_never_part_of_a_number() {
    assert_eq!(tokenize("-5"), Ok(vec![Token::Minus, num(5)]), "unary minus is a separate token");
    assert_eq!(tokenize("3-5"), Ok(vec![num(3), Token::Minus, num(5)]), "binary minus");
}

#[test]
fn unexpected_character_with_position() {
    assert_eq!(tokenize("4 $ 5"), Err(LexError::UnexpectedChar('$', 2)), "position of the dollar sign");
    assert_eq!(tokenize("ab#"), Err(LexError::UnexpectedChar('#', 2)), "error straight after a name");
    assert_eq!(tokenize("12."), Err(LexError::UnexpectedChar('.', 2)), "error straight after a number");
}

#[test]
fn first_error_is_reported() {
    assert_eq!(tokenize("a ? b $"), Err(LexError::UnexpectedChar('?', 2)), "the first bad character wins");
}

#[test]
fn byte_offsets_count_bytes() {
    // 'é' is two bytes in UTF-8, so '$' sits at byte offset 3
    assert_eq!(tokenize("é $"), Err(LexError::UnexpectedChar('$', 3)), "offsets are bytes, not characters");
}

#[test]
fn numbers_too_large() {
    assert_eq!(tokenize("9223372036854775807"), Ok(vec![num(i64::MAX)]), "the largest i64 fits");
    assert_eq!(tokenize("1 + 9223372036854775808"), Err(LexError::BadNumber(4)), "one more does not");
    assert_eq!(tokenize("99999999999999999999 x"), Err(LexError::BadNumber(0)), "far too large");
}
