fn middle(v: &[i32]) -> &[i32] {
    if v.len() <= 2 {
        &[]
    } else {
        &v[1..v.len() - 1]
    }
}

fn last_word(s: &str) -> &str {
    s.split_whitespace().last().unwrap_or("")
}

fn split_at_char(s: &str, c: char) -> (&str, &str) {
    match s.find(c) {
        Some(i) => (&s[..i], &s[i + c.len_utf8()..]),
        None => (s, ""),
    }
}

fn join_dash(parts: &[&str]) -> String {
    parts.join("-")
}

fn main() {
    println!("{:?}", middle(&[1, 2, 3, 4]));
}
