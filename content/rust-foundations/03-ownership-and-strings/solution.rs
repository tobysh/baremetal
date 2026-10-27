fn reverse(s: &str) -> String {
    s.chars().rev().collect()
}

fn first_word(s: &str) -> &str {
    match s.find(' ') {
        Some(i) => &s[..i],
        None => s,
    }
}

fn main() {
    println!("{}", reverse("héllo"));
    println!("{}", first_word("hello world"));
}
