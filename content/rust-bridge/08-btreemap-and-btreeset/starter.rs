use std::collections::{BTreeMap, BTreeSet};

fn index_words(lines: &[&str]) -> BTreeMap<String, Vec<usize>> {
    BTreeMap::new()
}

fn alphabetical(index: &BTreeMap<String, Vec<usize>>) -> Vec<String> {
    Vec::new()
}

fn between(prices: &BTreeMap<u32, String>, lo: u32, hi: u32) -> Vec<String> {
    Vec::new()
}

fn cheapest_at_least(prices: &BTreeMap<u32, String>, min: u32) -> Option<&String> {
    None
}

fn priciest_below(prices: &BTreeMap<u32, String>, limit: u32) -> Option<&String> {
    None
}

fn distinct_gaps(v: &[i32]) -> BTreeSet<i32> {
    BTreeSet::new()
}

// Try it with Run.
fn main() {
    let idx = index_words(&["the cat", "the dog the end", "cat"]);
    println!("{:?}", idx);
    println!("{:?}", alphabetical(&idx));
    let mut p = BTreeMap::new();
    p.insert(5, "pen".to_string());
    p.insert(20, "book".to_string());
    p.insert(50, "bag".to_string());
    println!("{:?}", between(&p, 5, 20));
    println!("{:?} {:?}", cheapest_at_least(&p, 6), priciest_below(&p, 50));
    println!("{:?}", distinct_gaps(&[1, 4, 4, 7, 8]));
}
