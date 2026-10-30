fn middle(v: &[i32]) -> &[i32] {
    v
}

fn last_word(s: &str) -> &str {
    s
}

fn split_at_char(s: &str, c: char) -> (&str, &str) {
    (s, "")
}

fn join_dash(parts: &[&str]) -> String {
    String::new()
}

// Try your functions with Run.
fn main() {
    println!("{:?}", middle(&[1, 2, 3, 4]));
    println!("{:?}", last_word("the quick fox"));
    println!("{:?}", split_at_char("key=value", '='));
    println!("{:?}", join_dash(&["a", "b"]));
}
