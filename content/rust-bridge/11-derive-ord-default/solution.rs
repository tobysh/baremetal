#[derive(Debug, Clone, PartialEq, Eq, Hash, PartialOrd, Ord)]
struct Version {
    major: u32,
    minor: u32,
    patch: u32,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
enum Suit {
    Clubs,
    Diamonds,
    Hearts,
    Spades,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
struct Card {
    rank: u8,
    suit: Suit,
}

#[derive(Debug, Default, Clone, PartialEq)]
struct Settings {
    name: String,
    retries: u32,
    verbose: bool,
    tags: Vec<String>,
}

fn parse_version(s: &str) -> Option<Version> {
    let parts: Vec<&str> = s.split('.').collect();
    if parts.len() != 3 {
        return None;
    }
    Some(Version {
        major: parts[0].parse().ok()?,
        minor: parts[1].parse().ok()?,
        patch: parts[2].parse().ok()?,
    })
}

fn newest(versions: &[Version]) -> Option<Version> {
    versions.iter().max().cloned()
}

fn dedup_versions(v: &[Version]) -> Vec<Version> {
    let mut out = v.to_vec();
    out.sort();
    out.dedup();
    out
}

fn sorted_hand(cards: &[Card]) -> Vec<Card> {
    let mut out = cards.to_vec();
    out.sort();
    out
}

fn quiet_settings(name: &str) -> Settings {
    Settings {
        name: name.to_string(),
        retries: 3,
        ..Settings::default()
    }
}

fn needs_bump(a: &Version, b: &Version) -> bool {
    (b.major, b.minor) > (a.major, a.minor)
}

fn main() {
    println!("{:?}", parse_version("1.12.3"));
}
