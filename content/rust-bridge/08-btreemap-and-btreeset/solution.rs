use std::collections::{BTreeMap, BTreeSet};

fn index_words(lines: &[&str]) -> BTreeMap<String, Vec<usize>> {
    let mut m: BTreeMap<String, Vec<usize>> = BTreeMap::new();
    for (i, line) in lines.iter().enumerate() {
        let words: BTreeSet<String> = line.split_whitespace().map(|w| w.to_lowercase()).collect();
        for w in words {
            m.entry(w).or_default().push(i + 1);
        }
    }
    m
}

fn alphabetical(index: &BTreeMap<String, Vec<usize>>) -> Vec<String> {
    index
        .iter()
        .filter(|(_, l)| l.len() > 1)
        .map(|(w, _)| w.clone())
        .collect()
}

fn between(prices: &BTreeMap<u32, String>, lo: u32, hi: u32) -> Vec<String> {
    if lo > hi {
        return Vec::new();
    }
    prices.range(lo..=hi).map(|(_, n)| n.clone()).collect()
}

fn cheapest_at_least(prices: &BTreeMap<u32, String>, min: u32) -> Option<&String> {
    prices.range(min..).next().map(|(_, n)| n)
}

fn priciest_below(prices: &BTreeMap<u32, String>, limit: u32) -> Option<&String> {
    prices.range(..limit).next_back().map(|(_, n)| n)
}

fn distinct_gaps(v: &[i32]) -> BTreeSet<i32> {
    let s: BTreeSet<i32> = v.iter().copied().collect();
    let sorted: Vec<i32> = s.into_iter().collect();
    sorted.windows(2).map(|w| w[1] - w[0]).collect()
}

fn main() {
    println!("{:?}", distinct_gaps(&[1, 4, 4, 7, 8]));
}
