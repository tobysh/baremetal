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
        let s = self.rest.trim_start();
        if s.is_empty() {
            self.rest = s;
            return None;
        }
        let end = s.find(char::is_whitespace).unwrap_or(s.len());
        let (tok, rest) = s.split_at(end);
        self.rest = rest;
        Some(tok)
    }
}

#[derive(Debug, PartialEq)]
struct Record<'a> {
    key: &'a str,
    value: &'a str,
}

fn parse_record<'a>(line: &'a str) -> Option<Record<'a>> {
    let (k, v) = line.split_once('=')?;
    let key = k.trim();
    if key.is_empty() {
        return None;
    }
    Some(Record { key, value: v.trim() })
}

fn parse_all<'a>(text: &'a str) -> Vec<Record<'a>> {
    text.lines().filter_map(parse_record).collect()
}

fn longest_value<'a>(records: &[Record<'a>]) -> Option<&'a str> {
    let mut best: Option<&'a str> = None;
    for r in records {
        match best {
            Some(b) if b.len() >= r.value.len() => {}
            _ => best = Some(r.value),
        }
    }
    best
}

// Try it with Run.
fn main() {
    let words: Vec<&str> = Tokens::new("  to be  or\tnot").collect();
    println!("{:?}", words);
    println!("{:?}", parse_all("a = 1\nbad line\nname = Ada"));
}
