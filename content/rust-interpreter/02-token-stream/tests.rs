fn stream(src: &str) -> Stream {
    Stream::from_source(src).unwrap()
}

#[test]
fn peek_does_not_consume() {
    let s = stream("1 + 2");
    assert_eq!(s.peek(), Some(&Token::Num(1)), "first peek");
    assert_eq!(s.peek(), Some(&Token::Num(1)), "second peek gives the same token");
}

#[test]
fn peek_on_empty_stream() {
    let s = stream("");
    assert_eq!(s.peek(), None, "no tokens");
    assert!(s.at_end(), "an empty stream is at its end");
    assert_eq!(s.peek_at(0), None, "peek_at(0) on an empty stream");
}

#[test]
fn peek_at_looks_ahead() {
    let s = stream("a = 5 ;");
    assert_eq!(s.peek_at(0), Some(&Token::Ident("a".to_string())), "peek_at(0) is peek");
    assert_eq!(s.peek_at(1), Some(&Token::Eq), "one ahead");
    assert_eq!(s.peek_at(3), Some(&Token::Semi), "three ahead");
    assert_eq!(s.peek_at(4), None, "past the end");
    assert_eq!(s.peek_at(100), None, "far past the end");
}

#[test]
fn next_consumes_in_order() {
    let mut s = stream("1 + 2");
    assert_eq!(s.next(), Some(Spanned { tok: Token::Num(1), pos: 0 }), "first token with its position");
    assert_eq!(s.peek(), Some(&Token::Plus), "after next, peek sees the following token");
    assert_eq!(s.next(), Some(Spanned { tok: Token::Plus, pos: 2 }), "second token");
    assert_eq!(s.peek_at(0), Some(&Token::Num(2)), "lookahead is relative to the cursor");
    assert_eq!(s.next(), Some(Spanned { tok: Token::Num(2), pos: 4 }), "third token");
    assert!(s.at_end(), "all consumed");
    assert_eq!(s.next(), None, "next at the end gives None");
    assert_eq!(s.next(), None, "and keeps giving None");
}

#[test]
fn at_end_flips_after_last_token() {
    let mut s = stream("x");
    assert!(!s.at_end(), "one token left");
    s.next();
    assert!(s.at_end(), "none left");
}

#[test]
fn pos_is_next_token_or_end_of_source() {
    let mut s = stream("ab  + c");
    assert_eq!(s.pos(), 0, "ab");
    s.next();
    assert_eq!(s.pos(), 4, "+");
    s.next();
    assert_eq!(s.pos(), 6, "c");
    s.next();
    assert_eq!(s.pos(), 7, "at the end, pos is the source length");
}

#[test]
fn pos_at_end_counts_trailing_whitespace() {
    let mut s = stream("x   ");
    s.next();
    assert_eq!(s.pos(), 4, "end of input is the length of the source text, not of the last token");
}

#[test]
fn eat_consumes_only_on_match() {
    let mut s = stream("( x )");
    assert!(!s.eat(&Token::RParen), "next token is ( not )");
    assert_eq!(s.peek(), Some(&Token::LParen), "a failed eat consumes nothing");
    assert!(s.eat(&Token::LParen), "( matches");
    assert_eq!(s.peek(), Some(&Token::Ident("x".to_string())), "and was consumed");
    assert!(!s.eat(&Token::Num(1)), "different token");
    s.next();
    assert!(s.eat(&Token::RParen), ") matches");
    assert!(!s.eat(&Token::RParen), "eat at the end is false");
}

#[test]
fn eat_compares_payloads() {
    let mut s = stream("7");
    assert!(!s.eat(&Token::Num(8)), "Num(8) is not Num(7)");
    assert!(s.eat(&Token::Num(7)), "Num(7) is");
}

#[test]
fn expect_success_returns_position() {
    let mut s = stream("f ( )");
    s.next();
    assert_eq!(s.expect(&Token::LParen), Ok(2), "( is at offset 2");
    assert_eq!(s.expect(&Token::RParen), Ok(4), ") is at offset 4");
    assert!(s.at_end(), "both consumed");
}

#[test]
fn expect_wrong_token() {
    let mut s = stream("a + b");
    s.next();
    let err = s.expect(&Token::Semi).unwrap_err();
    assert_eq!(err.msg, "expected Semi, found Plus", "message names both tokens (Debug format)");
    assert_eq!(err.pos, 2, "error position is where the wrong token is");
    assert_eq!(s.peek(), Some(&Token::Plus), "a failed expect consumes nothing");
}

#[test]
fn expect_wrong_token_with_payload() {
    let mut s = stream("   12");
    let err = s.expect(&Token::RParen).unwrap_err();
    assert_eq!(err.msg, "expected RParen, found Num(12)", "payloads appear in the message");
    assert_eq!(err.pos, 3, "position of the 12");
}

#[test]
fn expect_at_end_of_input() {
    let mut s = stream("(1 ");
    s.next();
    s.next();
    let err = s.expect(&Token::RParen).unwrap_err();
    assert_eq!(err.msg, "expected RParen, found end of input", "nothing left");
    assert_eq!(err.pos, 3, "end of input is the source length");
}

#[test]
fn expect_ident_works() {
    let mut s = stream("count = 1");
    assert_eq!(s.expect_ident(), Ok("count".to_string()), "returns the name");
    assert_eq!(s.peek(), Some(&Token::Eq), "and consumed it");
}

#[test]
fn expect_ident_errors() {
    let mut s = stream("let 5");
    s.next();
    let err = s.expect_ident().unwrap_err();
    assert_eq!(err.msg, "expected identifier, found Num(5)", "wrong token");
    assert_eq!(err.pos, 4, "position of 5");
    assert_eq!(s.peek(), Some(&Token::Num(5)), "nothing consumed");
    let mut s = stream("let");
    s.next();
    let err = s.expect_ident().unwrap_err();
    assert_eq!(err.msg, "expected identifier, found end of input", "end of input");
    assert_eq!(err.pos, 3, "at the end");
    let mut s = stream("fn");
    let err = s.expect_ident().unwrap_err();
    assert_eq!(err.msg, "expected identifier, found Fn", "a keyword is not an identifier");
}

#[test]
fn from_source_reports_lex_errors_as_parse_errors() {
    let err = Stream::from_source("1 + $").err().unwrap();
    assert_eq!(err, ParseError { msg: "unexpected character '$'".to_string(), pos: 4 }, "given conversion");
}

#[test]
fn a_tiny_parse_using_the_stream() {
    // sum of a,b,c written as "a + b + c": consume with eat/peek only
    let mut s = stream("a + b + c");
    let mut names = vec![s.expect_ident().unwrap()];
    while s.eat(&Token::Plus) {
        names.push(s.expect_ident().unwrap());
    }
    assert_eq!(names, vec!["a", "b", "c"], "names collected");
    assert!(s.at_end(), "all input used");
}
