fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    if b.len() > a.len() {
        b
    } else {
        a
    }
}

fn first_word(s: &str) -> &str {
    s.split_whitespace().next().unwrap_or("")
}

fn split_pair<'a>(s: &'a str, sep: char) -> Option<(&'a str, &'a str)> {
    let i = s.find(sep)?;
    Some((&s[..i], &s[i + sep.len_utf8()..]))
}

fn strip_known<'a>(text: &'a str, prefix: &str) -> &'a str {
    text.strip_prefix(prefix).unwrap_or(text)
}

fn longest_line(text: &str) -> &str {
    let mut best = "";
    for line in text.lines() {
        if line.len() > best.len() {
            best = line;
        }
    }
    best
}

// Try it with Run.
fn main() {
    println!("{}", longest("apple", "fig"));
    println!("{:?}", split_pair("key=value", '='));
    println!("{}", strip_known("v2:alpha", "v2:"));
}
