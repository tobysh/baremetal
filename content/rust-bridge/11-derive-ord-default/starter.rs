use std::cmp::Ordering;

// These hand-written comparisons are wrong (they ignore most fields).
// Delete each `impl` and let `derive` write the right one instead.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
struct Version {
    major: u32,
    minor: u32,
    patch: u32,
}

impl PartialOrd for Version {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}
impl Ord for Version {
    fn cmp(&self, other: &Self) -> Ordering {
        self.patch.cmp(&other.patch)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
enum Suit {
    Clubs,
    Diamonds,
    Hearts,
    Spades,
}

impl PartialOrd for Suit {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}
impl Ord for Suit {
    fn cmp(&self, _other: &Self) -> Ordering {
        Ordering::Equal
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
struct Card {
    rank: u8,
    suit: Suit,
}

impl PartialOrd for Card {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}
impl Ord for Card {
    fn cmp(&self, other: &Self) -> Ordering {
        self.rank.cmp(&other.rank)
    }
}

// This Default is wrong too: derive it instead.
#[derive(Debug, Clone, PartialEq)]
struct Settings {
    name: String,
    retries: u32,
    verbose: bool,
    tags: Vec<String>,
}

impl Default for Settings {
    fn default() -> Self {
        Settings { name: "unnamed".to_string(), retries: 1, verbose: true, tags: vec!["x".to_string()] }
    }
}

fn parse_version(s: &str) -> Option<Version> {
    None
}

fn newest(versions: &[Version]) -> Option<Version> {
    None
}

fn dedup_versions(v: &[Version]) -> Vec<Version> {
    Vec::new()
}

fn sorted_hand(cards: &[Card]) -> Vec<Card> {
    Vec::new()
}

fn quiet_settings(name: &str) -> Settings {
    Settings { name: String::new(), retries: 0, verbose: false, tags: Vec::new() }
}

fn needs_bump(a: &Version, b: &Version) -> bool {
    false
}

// Try it with Run.
fn main() {
    let v = parse_version("1.12.3").unwrap();
    println!("{}.{}.{}", v.major, v.minor, v.patch);
    let s = quiet_settings("demo");
    println!("{} {} {} {}", s.name, s.retries, s.verbose, s.tags.len());
}
