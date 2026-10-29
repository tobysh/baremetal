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

// ================= The token stream =================

#[derive(Debug, PartialEq, Clone)]
struct ParseError {
    msg: String,
    pos: usize,
}

impl ParseError {
    fn new(msg: impl Into<String>, pos: usize) -> ParseError {
        ParseError { msg: msg.into(), pos }
    }
}

impl From<LexError> for ParseError {
    fn from(e: LexError) -> ParseError {
        match e {
            LexError::UnexpectedChar(c, pos) => ParseError::new(format!("unexpected character '{}'", c), pos),
            LexError::NumberTooBig(pos) => ParseError::new("number too big", pos),
        }
    }
}

/// The tokens of one source text plus a cursor. `end` is the source length, used as the
/// position of "end of input".
struct Stream {
    toks: Vec<Spanned>,
    i: usize,
    end: usize,
}

impl Stream {
    fn new(toks: Vec<Spanned>, end: usize) -> Stream {
        Stream { toks, i: 0, end }
    }

    fn from_source(src: &str) -> Result<Stream, ParseError> {
        Ok(Stream::new(lex(src)?, src.len()))
    }

    /// The next token, without consuming it.
    fn peek(&self) -> Option<&Token> {
        self.toks.get(self.i).map(|s| &s.tok)
    }

    /// The token `n` places ahead (`peek_at(0)` is `peek()`).
    fn peek_at(&self, n: usize) -> Option<&Token> {
        self.toks.get(self.i + n).map(|s| &s.tok)
    }

    /// Byte offset of the next token, or the end of the source when there are no more.
    fn pos(&self) -> usize {
        self.toks.get(self.i).map_or(self.end, |s| s.pos)
    }

    /// Consume and return the next token.
    fn next(&mut self) -> Option<Spanned> {
        let t = self.toks.get(self.i).cloned();
        if t.is_some() {
            self.i += 1;
        }
        t
    }

    /// Consume the next token only if it equals `t`. Says whether it did.
    fn eat(&mut self, t: &Token) -> bool {
        if self.peek() == Some(t) {
            self.i += 1;
            true
        } else {
            false
        }
    }

    /// Consume `t` or fail. On success returns the token's position.
    fn expect(&mut self, t: &Token) -> Result<usize, ParseError> {
        let pos = self.pos();
        match self.peek() {
            Some(found) if found == t => {
                self.i += 1;
                Ok(pos)
            }
            Some(found) => Err(ParseError::new(format!("expected {:?}, found {:?}", t, found), pos)),
            None => Err(ParseError::new(format!("expected {:?}, found end of input", t), pos)),
        }
    }

    /// Consume an identifier and return its name.
    fn expect_ident(&mut self) -> Result<String, ParseError> {
        let pos = self.pos();
        match self.peek().cloned() {
            Some(Token::Ident(name)) => {
                self.i += 1;
                Ok(name)
            }
            Some(found) => Err(ParseError::new(format!("expected identifier, found {:?}", found), pos)),
            None => Err(ParseError::new("expected identifier, found end of input", pos)),
        }
    }

    fn at_end(&self) -> bool {
        self.i >= self.toks.len()
    }
}

// Try it with Run.
fn main() {
    let mut s = Stream::from_source("let x = 1;").unwrap();
    println!("{:?} / {:?}", s.peek(), s.peek_at(1));
    println!("{:?}", s.next());
    println!("{:?} then pos {}", s.expect(&Token::Semi), s.pos());
}
