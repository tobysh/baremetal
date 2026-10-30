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

// Write your State enum here.

fn tokenize(input: &str) -> Result<Vec<Token>, LexError> {
    Ok(Vec::new())
}

// Try it with Run.
fn main() {
    println!("{:?}", tokenize("12 + count * (3 - x)"));
    println!("{:?}", tokenize("4 $ 5"));
}
