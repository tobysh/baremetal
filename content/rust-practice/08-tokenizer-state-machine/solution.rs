#[derive(Debug, PartialEq, Clone)]
enum Token {
    Num(i64),
    Ident(String),
    Plus,
    Minus,
    Star,
    Slash,
    LParen,
    RParen,
}

#[derive(Debug, PartialEq, Clone)]
enum LexError {
    UnexpectedChar(char, usize),
    BadNumber(usize),
}

enum State {
    Start,
    Number { start: usize, text: String },
    Ident(String),
}

fn tokenize(input: &str) -> Result<Vec<Token>, LexError> {
    let chars: Vec<(usize, char)> = input.char_indices().collect();
    let mut tokens = Vec::new();
    let mut state = State::Start;
    let mut i = 0;

    loop {
        let cur = chars.get(i).copied();
        state = match (state, cur) {
            (State::Start, None) => break,
            (State::Start, Some((_, c))) if c.is_whitespace() => {
                i += 1;
                State::Start
            }
            (State::Start, Some((p, c))) if c.is_ascii_digit() => {
                i += 1;
                State::Number { start: p, text: c.to_string() }
            }
            (State::Start, Some((_, c))) if c.is_alphabetic() || c == '_' => {
                i += 1;
                State::Ident(c.to_string())
            }
            (State::Start, Some((p, c))) => {
                let token = match c {
                    '+' => Token::Plus,
                    '-' => Token::Minus,
                    '*' => Token::Star,
                    '/' => Token::Slash,
                    '(' => Token::LParen,
                    ')' => Token::RParen,
                    _ => return Err(LexError::UnexpectedChar(c, p)),
                };
                tokens.push(token);
                i += 1;
                State::Start
            }
            (State::Number { start, mut text }, Some((_, c))) if c.is_ascii_digit() => {
                text.push(c);
                i += 1;
                State::Number { start, text }
            }
            (State::Number { start, text }, _) => {
                let n: i64 = text.parse().map_err(|_| LexError::BadNumber(start))?;
                tokens.push(Token::Num(n));
                State::Start
            }
            (State::Ident(mut name), Some((_, c))) if c.is_alphanumeric() || c == '_' => {
                name.push(c);
                i += 1;
                State::Ident(name)
            }
            (State::Ident(name), _) => {
                tokens.push(Token::Ident(name));
                State::Start
            }
        };
    }
    Ok(tokens)
}

// Try it with Run.
fn main() {
    println!("{:?}", tokenize("12 + count * (3 - x)"));
    println!("{:?}", tokenize("4 $ 5"));
}
