fn sample() -> Vec<Entry> {
    let text = "ERROR net: connection lost\nwarn disk: almost full\nINFO net: up\nerror net: timeout after 30 s\nINFO ui:\nWARN disk:   second warning  \n";
    parse_all(text).0
}

#[test]
fn single_lines() {
    let e = parse_line("ERROR net: connection lost after 30 s").expect("valid line");
    assert_eq!((e.level.as_str(), e.module.as_str(), e.message.as_str()), ("ERROR", "net", "connection lost after 30 s"), "plain line");
    let e = parse_line("   warn   disk:    almost   full  ").expect("extra spaces");
    assert_eq!((e.level.as_str(), e.module.as_str(), e.message.as_str()), ("WARN", "disk", "almost   full"), "spaces and case");
    let e = parse_line("INFO ui:").expect("empty message");
    assert_eq!((e.module.as_str(), e.message.as_str()), ("ui", ""), "message may be empty");
}

#[test]
fn bad_single_lines() {
    assert!(parse_line("").is_none(), "empty");
    assert!(parse_line("ERROR").is_none(), "level only");
    assert!(parse_line("ERROR net connection").is_none(), "module has no colon");
    assert!(parse_line("ERROR : x").is_none(), "empty module");
    assert!(parse_line("ERROR :").is_none(), "empty module at end");
}

#[test]
fn whole_text() {
    let (entries, bad) = parse_all("INFO a: one\n\nbroken\nWARN b: two\n   \nnope nope\n");
    assert_eq!(entries.len(), 2, "two good lines");
    assert_eq!(bad, vec![3, 6], "bad lines are numbered, blanks counted");
    let (e2, b2) = parse_all("");
    assert!(e2.is_empty() && b2.is_empty(), "empty text");
}

#[test]
fn level_counts() {
    let m = count_levels(&sample());
    let pairs: Vec<(&str, usize)> = m.iter().map(|(k, v)| (k.as_str(), *v)).collect();
    assert_eq!(pairs, vec![("ERROR", 2), ("INFO", 2), ("WARN", 2)], "two of each, sorted by level");
    assert!(count_levels(&[]).is_empty(), "nothing to count");
}

#[test]
fn busiest() {
    assert_eq!(busiest_module(&sample()), Some(("net".to_string(), 3)), "net has three entries");
    let tie = parse_all("INFO b: x\nINFO a: y\n").0;
    assert_eq!(busiest_module(&tie), Some(("a".to_string(), 1)), "tie goes to the first alphabetically");
    assert_eq!(busiest_module(&[]), None, "no entries");
}

#[test]
fn messages() {
    assert_eq!(messages_for(&sample(), "warn"), vec!["almost full".to_string(), "second warning".to_string()], "case-insensitive, trimmed");
    assert_eq!(messages_for(&sample(), "INFO"), vec!["up".to_string()], "empty message skipped");
    assert!(messages_for(&sample(), "debug").is_empty(), "no such level");
}
