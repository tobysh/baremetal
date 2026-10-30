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

fn main() {
    let counts = word_count("The cat; the DOG. the end!");
    let mut words: Vec<_> = counts.iter().collect();
    words.sort();
    println!("{:?}", words);
}
