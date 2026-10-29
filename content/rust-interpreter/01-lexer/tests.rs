fn toks(src: &str) -> Vec<Token> {
    lex(src).unwrap().into_iter().map(|s| s.tok).collect()
}

#[test]
fn empty_and_blank_input() {
    assert_eq!(lex(""), Ok(vec![]), "empty source has no tokens");
    assert_eq!(lex("  \t\n  "), Ok(vec![]), "whitespace alone has no tokens");
}

#[test]
fn numbers() {
    assert_eq!(toks("0"), vec![Token::Num(0)], "0");
    assert_eq!(toks("42 7"), vec![Token::Num(42), Token::Num(7)], "two numbers");
    assert_eq!(toks("9223372036854775807"), vec![Token::Num(i64::MAX)], "the largest i64");
}

#[test]
fn number_too_big() {
    assert_eq!(lex("9223372036854775808"), Err(LexError::NumberTooBig(0)), "one past i64::MAX");
    assert_eq!(lex("1 + 99999999999999999999"), Err(LexError::NumberTooBig(4)), "position of the first digit");
}

#[test]
fn identifiers_and_keywords() {
    assert_eq!(toks("foo"), vec![Token::Ident("foo".to_string())], "plain identifier");
    assert_eq!(toks("_a1_b2"), vec![Token::Ident("_a1_b2".to_string())], "underscores and digits");
    assert_eq!(
        toks("let if else while fn return print true false"),
        vec![
            Token::Let,
            Token::If,
            Token::Else,
            Token::While,
            Token::Fn,
            Token::Return,
            Token::Print,
            Token::True,
            Token::False
        ],
        "all nine keywords"
    );
    assert_eq!(toks("letter iffy"), vec![Token::Ident("letter".to_string()), Token::Ident("iffy".to_string())], "keywords only match whole words");
    assert_eq!(toks("x1"), vec![Token::Ident("x1".to_string())], "a digit inside a name stays in the name");
}

#[test]
fn single_char_operators() {
    assert_eq!(
        toks("+ - * / % ! = < > ( ) { } , ;"),
        vec![
            Token::Plus,
            Token::Minus,
            Token::Star,
            Token::Slash,
            Token::Percent,
            Token::Bang,
            Token::Eq,
            Token::Lt,
            Token::Gt,
            Token::LParen,
            Token::RParen,
            Token::LBrace,
            Token::RBrace,
            Token::Comma,
            Token::Semi
        ],
        "every one-character token"
    );
}

#[test]
fn two_char_operators_need_one_char_lookahead() {
    assert_eq!(toks("== != <= >="), vec![Token::EqEq, Token::BangEq, Token::LtEq, Token::GtEq], "two-character operators");
    assert_eq!(toks("= ="), vec![Token::Eq, Token::Eq], "separated equals signs are two tokens");
    assert_eq!(toks("==="), vec![Token::EqEq, Token::Eq], "=== is == then =");
    assert_eq!(toks("a<=b"), vec![Token::Ident("a".to_string()), Token::LtEq, Token::Ident("b".to_string())], "no spaces needed");
    assert_eq!(toks("!!x"), vec![Token::Bang, Token::Bang, Token::Ident("x".to_string())], "two bangs");
    assert_eq!(toks("!="), vec![Token::BangEq], "!=");
    assert_eq!(toks("<"), vec![Token::Lt], "< at the very end of the input");
}

#[test]
fn positions_are_byte_offsets() {
    let v = lex("let x = 12;").unwrap();
    let got: Vec<(Token, usize)> = v.into_iter().map(|s| (s.tok, s.pos)).collect();
    assert_eq!(
        got,
        vec![
            (Token::Let, 0),
            (Token::Ident("x".to_string()), 4),
            (Token::Eq, 6),
            (Token::Num(12), 8),
            (Token::Semi, 10)
        ],
        "each token records where it starts"
    );
    let v = lex("a >= b").unwrap();
    assert_eq!(v[1].pos, 2, ">= starts at its first character");
    assert_eq!(v[2].pos, 5, "b is after the two-character operator");
}

#[test]
fn positions_across_lines() {
    let v = lex("a\n  b").unwrap();
    assert_eq!(v[0].pos, 0, "a");
    assert_eq!(v[1].pos, 4, "b follows a newline and two spaces");
}

#[test]
fn comments_are_skipped() {
    assert_eq!(toks("1 # one\n2"), vec![Token::Num(1), Token::Num(2)], "a comment runs to the end of the line");
    assert_eq!(toks("# only a comment"), vec![], "a comment may be the whole input");
    assert_eq!(lex("x # c\ny").unwrap()[1].pos, 6, "positions after a comment still count its bytes");
}

#[test]
fn unexpected_characters() {
    assert_eq!(lex("1 $ 2"), Err(LexError::UnexpectedChar('$', 2)), "dollar sign");
    assert_eq!(lex("a & b"), Err(LexError::UnexpectedChar('&', 2)), "a single & is not an operator");
    assert_eq!(lex("let s = \"hi\";"), Err(LexError::UnexpectedChar('"', 8)), "there are no strings");
    assert_eq!(lex("x.y"), Err(LexError::UnexpectedChar('.', 1)), "dot");
}

#[test]
fn error_position_counts_bytes_not_chars() {
    assert_eq!(lex("é"), Err(LexError::UnexpectedChar('é', 0)), "a non-ASCII letter is not an identifier");
    assert_eq!(lex("# é\n@"), Err(LexError::UnexpectedChar('@', 5)), "é is two bytes, so @ is at offset 5");
}

#[test]
fn a_whole_statement() {
    assert_eq!(
        toks("while i<10 { i = i+1; }"),
        vec![
            Token::While,
            Token::Ident("i".to_string()),
            Token::Lt,
            Token::Num(10),
            Token::LBrace,
            Token::Ident("i".to_string()),
            Token::Eq,
            Token::Ident("i".to_string()),
            Token::Plus,
            Token::Num(1),
            Token::Semi,
            Token::RBrace
        ],
        "while i<10 {{ i = i+1; }}"
    );
}
