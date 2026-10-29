use std::collections::HashMap;

fn word_count(text: &str) -> HashMap<String, usize> {
    let mut counts = HashMap::new();
    for raw in text.split_whitespace() {
        let lower = raw.to_lowercase();
        let w = lower.trim_matches(|c: char| c.is_ascii_punctuation());
        if w.is_empty() {
            continue;
        }
        *counts.entry(w.to_string()).or_insert(0) += 1;
    }
    counts
}

fn top_words(text: &str, n: usize) -> Vec<(String, usize)> {
    let mut pairs: Vec<(String, usize)> = word_count(text).into_iter().collect();
    pairs.sort_by(|a, b| b.1.cmp(&a.1).then_with(|| a.0.cmp(&b.0)));
    pairs.truncate(n);
    pairs
}

// Try it with Run.
fn main() {
    println!("{:?}", top_words("the cat and the hat and the bat", 2));
}
