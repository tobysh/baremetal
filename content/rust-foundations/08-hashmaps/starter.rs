use std::collections::HashMap;

fn word_count(text: &str) -> HashMap<String, usize> {
    HashMap::new()
}

// Try your function with Run.
fn main() {
    let counts = word_count("The cat; the DOG. the end!");
    let mut words: Vec<_> = counts.iter().collect();
    words.sort();
    println!("{:?}", words);
}
