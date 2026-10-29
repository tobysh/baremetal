fn v(a: u32, b: u32, c: u32) -> Version {
    Version { major: a, minor: b, patch: c }
}

#[test]
fn parsing() {
    assert_eq!(parse_version("1.12.3"), Some(v(1, 12, 3)), "normal");
    assert_eq!(parse_version("0.0.0"), Some(v(0, 0, 0)), "zeros");
    assert_eq!(parse_version("10.0.200"), Some(v(10, 0, 200)), "wide numbers");
    for bad in ["1.2", "1.2.3.4", "a.b.c", "1..3", "", "1.2.-3", "1.2.3 "] {
        assert_eq!(parse_version(bad), None, "{:?} must not parse", bad);
    }
}

#[test]
fn ordering_follows_field_order() {
    assert!(v(1, 9, 0) < v(2, 0, 0), "major wins over minor");
    assert!(v(1, 2, 9) < v(1, 3, 0), "minor wins over patch");
    assert!(v(1, 2, 3) == v(1, 2, 3), "equality");
    assert!(v(1, 10, 0) > v(1, 9, 99), "numeric, not textual");
}

#[test]
fn newest_and_dedup() {
    assert_eq!(newest(&[v(1, 0, 0), v(1, 10, 0), v(1, 9, 9)]), Some(v(1, 10, 0)), "newest of three");
    assert_eq!(newest(&[]), None, "empty");
    let d = dedup_versions(&[v(2, 0, 0), v(1, 0, 0), v(2, 0, 0), v(1, 5, 0)]);
    assert_eq!(d, vec![v(1, 0, 0), v(1, 5, 0), v(2, 0, 0)], "sorted without duplicates");
    let mut m = std::collections::HashMap::new();
    m.insert(v(1, 0, 0), "one");
    assert_eq!(m.get(&v(1, 0, 0)), Some(&"one"), "Version works as a map key");
}

#[test]
fn card_sorting() {
    let hand = [
        Card { rank: 10, suit: Suit::Spades },
        Card { rank: 3, suit: Suit::Hearts },
        Card { rank: 10, suit: Suit::Clubs },
        Card { rank: 3, suit: Suit::Clubs },
    ];
    let s = sorted_hand(&hand);
    let got: Vec<(u8, Suit)> = s.iter().map(|c| (c.rank, c.suit)).collect();
    assert_eq!(
        got,
        vec![(3, Suit::Clubs), (3, Suit::Hearts), (10, Suit::Clubs), (10, Suit::Spades)],
        "rank first, then suit in declared order"
    );
    assert!(Suit::Clubs < Suit::Spades, "variants compare by position");
}

#[test]
fn settings_defaults() {
    let s = quiet_settings("demo");
    assert_eq!(s.name, "demo", "name set");
    assert_eq!(s.retries, 3, "retries set");
    assert!(!s.verbose && s.tags.is_empty(), "the rest keep default values");
    let d = Settings::default();
    assert!(d.name.is_empty() && d.retries == 0, "Default exists");
}

#[test]
fn bumps() {
    assert!(needs_bump(&v(1, 2, 3), &v(1, 3, 0)), "minor bump");
    assert!(needs_bump(&v(1, 9, 9), &v(2, 0, 0)), "major bump");
    assert!(!needs_bump(&v(1, 2, 3), &v(1, 2, 9)), "patch only");
    assert!(!needs_bump(&v(2, 0, 0), &v(1, 9, 0)), "downgrade");
    assert!(!needs_bump(&v(1, 2, 3), &v(1, 2, 3)), "same");
}
