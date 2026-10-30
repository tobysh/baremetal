fn common_prefix<'a>(words: &[&'a str]) -> &'a str {
    let first = match words.first() {
        Some(w) => *w,
        None => return "",
    };
    let mut end = first.len();
    for w in &words[1..] {
        let mut shared = 0;
        for ((i, a), b) in first.char_indices().zip(w.chars()) {
            if a != b {
                break;
            }
            shared = i + a.len_utf8();
        }
        end = end.min(shared);
    }
    &first[..end]
}

// Try it with Run.
fn main() {
    println!("{:?}", common_prefix(&["flower", "flow", "flight"]));
}
