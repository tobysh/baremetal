fn common_prefix<'a>(words: &[&'a str]) -> &'a str {
    ""
}

// Try it with Run.
fn main() {
    println!("{:?}", common_prefix(&["flower", "flow", "flight"]));
}
