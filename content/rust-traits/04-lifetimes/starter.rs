fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    a
}

fn first_word(s: &str) -> &str {
    s
}

struct Words<'a> {
    rest: &'a str,
}

impl<'a> Words<'a> {
    fn new(text: &'a str) -> Words<'a> {
        Words { rest: text }
    }

    fn next_word(&mut self) -> Option<&'a str> {
        None
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
