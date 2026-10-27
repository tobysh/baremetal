fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    if a.len() >= b.len() {
        a
    } else {
        b
    }
}

fn first_word(s: &str) -> &str {
    match s.find(' ') {
        Some(i) => &s[..i],
        None => s,
    }
}

struct Words<'a> {
    rest: &'a str,
}

impl<'a> Words<'a> {
    fn new(text: &'a str) -> Words<'a> {
        Words { rest: text }
    }

    fn next_word(&mut self) -> Option<&'a str> {
        let trimmed = self.rest.trim_start();
        if trimmed.is_empty() {
            self.rest = trimmed;
            return None;
        }
        let end = trimmed.find(' ').unwrap_or(trimmed.len());
        let (word, rest) = trimmed.split_at(end);
        self.rest = rest;
        Some(word)
    }
}

fn main() {
    println!("{}", longest("tower", "keep"));
    println!("{}", first_word("hello world"));
    let mut w = Words::new("one two  three");
    while let Some(word) = w.next_word() {
        println!("[{}]", word);
    }
}
