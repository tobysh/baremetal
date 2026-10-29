use std::cell::RefCell;
use std::collections::HashMap;
use std::fmt;
use std::rc::Rc;

// ================= Tokens and the lexer =================

#[derive(Debug, PartialEq, Clone)]
enum Token {
    Num(i64),
    Ident(String),
    // keywords
    Let,
    If,
    Else,
    While,
    Fn,
    Return,
    Print,
    True,
    False,
    // operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Bang,
    Eq,     // =
    EqEq,   // ==
    BangEq, // !=
    Lt,
    LtEq,
    Gt,
    GtEq,
    // punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,
    Semi,
}

/// A token and the byte offset where it starts in the source.
#[derive(Debug, PartialEq, Clone)]
struct Spanned {
    tok: Token,
    pos: usize,
}

#[derive(Debug, PartialEq, Clone)]
enum LexError {
    UnexpectedChar(char, usize),
    NumberTooBig(usize),
}

fn lex(src: &str) -> Result<Vec<Spanned>, LexError> {
    let chars: Vec<(usize, char)> = src.char_indices().collect();
    let mut out = Vec::new();
    let mut i = 0;
    while i < chars.len() {
        let (pos, c) = chars[i];
        if c.is_whitespace() {
            i += 1;
            continue;
        }
        if c == '#' {
            while i < chars.len() && chars[i].1 != '\n' {
                i += 1;
            }
            continue;
        }
        if c.is_ascii_digit() {
            let start = i;
            while i < chars.len() && chars[i].1.is_ascii_digit() {
                i += 1;
            }
            let text: String = chars[start..i].iter().map(|&(_, d)| d).collect();
            let n = text.parse::<i64>().map_err(|_| LexError::NumberTooBig(pos))?;
            out.push(Spanned { tok: Token::Num(n), pos });
            continue;
        }
        if c.is_ascii_alphabetic() || c == '_' {
            let start = i;
            while i < chars.len() && (chars[i].1.is_ascii_alphanumeric() || chars[i].1 == '_') {
                i += 1;
            }
            let word: String = chars[start..i].iter().map(|&(_, d)| d).collect();
            let tok = match word.as_str() {
                "let" => Token::Let,
                "if" => Token::If,
                "else" => Token::Else,
                "while" => Token::While,
                "fn" => Token::Fn,
                "return" => Token::Return,
                "print" => Token::Print,
                "true" => Token::True,
                "false" => Token::False,
                _ => Token::Ident(word),
            };
            out.push(Spanned { tok, pos });
            continue;
        }
        let next = chars.get(i + 1).map(|&(_, n)| n);
        let (tok, len) = match (c, next) {
            ('=', Some('=')) => (Token::EqEq, 2),
            ('!', Some('=')) => (Token::BangEq, 2),
            ('<', Some('=')) => (Token::LtEq, 2),
            ('>', Some('=')) => (Token::GtEq, 2),
            ('=', _) => (Token::Eq, 1),
            ('!', _) => (Token::Bang, 1),
            ('<', _) => (Token::Lt, 1),
            ('>', _) => (Token::Gt, 1),
            ('+', _) => (Token::Plus, 1),
            ('-', _) => (Token::Minus, 1),
            ('*', _) => (Token::Star, 1),
            ('/', _) => (Token::Slash, 1),
            ('%', _) => (Token::Percent, 1),
            ('(', _) => (Token::LParen, 1),
            (')', _) => (Token::RParen, 1),
            ('{', _) => (Token::LBrace, 1),
            ('}', _) => (Token::RBrace, 1),
            (',', _) => (Token::Comma, 1),
            (';', _) => (Token::Semi, 1),
            _ => return Err(LexError::UnexpectedChar(c, pos)),
        };
        out.push(Spanned { tok, pos });
        i += len;
    }
    Ok(out)
}

// Try it with Run.
fn main() {
    println!("{:?}", lex("let x = 4 + 20;"));
    println!("{:?}", lex("a <= b != c"));
    println!("{:?}", lex("1 $ 2"));
}
