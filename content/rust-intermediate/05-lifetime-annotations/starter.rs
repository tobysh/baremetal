fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    ""
}

fn first_word(s: &str) -> &str {
    "?"
}

fn split_pair<'a>(s: &'a str, sep: char) -> Option<(&'a str, &'a str)> {
    None
}

fn strip_known<'a>(text: &'a str, prefix: &str) -> &'a str {
    ""
}

fn longest_line(text: &str) -> &str {
    "?"
}

// Try it with Run.
fn main() {
    println!("{}", longest("apple", "fig"));
    println!("{:?}", split_pair("key=value", '='));
    println!("{}", strip_known("v2:alpha", "v2:"));
}
