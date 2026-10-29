use std::collections::{HashMap, HashSet};

fn first_duplicate(v: &[i32]) -> Option<i32> {
    let mut seen = HashSet::new();
    for &x in v {
        if !seen.insert(x) {
            return Some(x);
        }
    }
    None
}

fn word_counts(text: &str) -> HashMap<String, usize> {
    let mut m = HashMap::new();
    for w in text.split_whitespace() {
        *m.entry(w.to_lowercase()).or_insert(0) += 1;
    }
    m
}

fn group_by_first_letter(words: &[&str]) -> HashMap<char, Vec<String>> {
    let mut m: HashMap<char, Vec<String>> = HashMap::new();
    for w in words {
        if let Some(c) = w.chars().next() {
            let key = c.to_lowercase().next().unwrap();
            m.entry(key).or_default().push(w.to_string());
        }
    }
    m
}

fn common_sorted(a: &[i32], b: &[i32]) -> Vec<i32> {
    let sa: HashSet<i32> = a.iter().copied().collect();
    let sb: HashSet<i32> = b.iter().copied().collect();
    let mut out: Vec<i32> = sa.intersection(&sb).copied().collect();
    out.sort();
    out
}

fn only_in_first_sorted(a: &[i32], b: &[i32]) -> Vec<i32> {
    let sa: HashSet<i32> = a.iter().copied().collect();
    let sb: HashSet<i32> = b.iter().copied().collect();
    let mut out: Vec<i32> = sa.difference(&sb).copied().collect();
    out.sort();
    out
}

fn most_common(text: &str) -> Option<(String, usize)> {
    word_counts(text)
        .into_iter()
        .min_by_key(|(w, c)| (std::cmp::Reverse(*c), w.clone()))
}

fn main() {
    println!("{:?}", most_common("a b b a c"));
}
