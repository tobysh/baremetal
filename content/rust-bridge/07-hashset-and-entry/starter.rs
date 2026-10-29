use std::collections::{HashMap, HashSet};

fn first_duplicate(v: &[i32]) -> Option<i32> {
    None
}

fn word_counts(text: &str) -> HashMap<String, usize> {
    HashMap::new()
}

fn group_by_first_letter(words: &[&str]) -> HashMap<char, Vec<String>> {
    HashMap::new()
}

fn common_sorted(a: &[i32], b: &[i32]) -> Vec<i32> {
    Vec::new()
}

fn only_in_first_sorted(a: &[i32], b: &[i32]) -> Vec<i32> {
    Vec::new()
}

fn most_common(text: &str) -> Option<(String, usize)> {
    None
}

// Try it with Run.
fn main() {
    println!("{:?}", first_duplicate(&[1, 2, 3, 2, 1]));
    let c = word_counts("the cat The dog");
    println!("the = {:?}", c.get("the"));
    println!("{:?}", common_sorted(&[1, 2, 3, 3], &[3, 2, 9]));
    println!("{:?}", only_in_first_sorted(&[1, 2, 3, 3], &[3, 2, 9]));
    println!("{:?}", most_common("a b b a c"));
    let _ = group_by_first_letter(&["apple", "Avocado", "bean"]);
    let _: HashSet<i32> = HashSet::new();
}
