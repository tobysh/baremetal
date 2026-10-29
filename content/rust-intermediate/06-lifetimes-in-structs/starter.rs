struct Tokens<'a> {
    rest: &'a str,
}

impl<'a> Tokens<'a> {
    fn new(text: &'a str) -> Self {
        Tokens { rest: text }
    }
}

impl<'a> Iterator for Tokens<'a> {
    type Item = &'a str;
    fn next(&mut self) -> Option<&'a str> {
        None
    }
}

#[derive(Debug, PartialEq)]
struct Record<'a> {
    key: &'a str,
    value: &'a str,
}

fn parse_record<'a>(line: &'a str) -> Option<Record<'a>> {
    None
}

fn parse_all<'a>(text: &'a str) -> Vec<Record<'a>> {
    Vec::new()
}

fn longest_value<'a>(records: &[Record<'a>]) -> Option<&'a str> {
    None
}

// Try it with Run.
fn main() {
    let words: Vec<&str> = Tokens::new("  to be  or\tnot").collect();
    println!("{:?}", words);
    println!("{:?}", parse_all("a = 1\nbad line\nname = Ada"));
}
