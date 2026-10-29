use std::collections::BTreeMap;

struct Entry {
    level: String,
    module: String,
    message: String,
}

fn parse_line(line: &str) -> Option<Entry> {
    let line = line.trim();
    let (level, rest) = line.split_once(char::is_whitespace)?;
    let rest = rest.trim_start();
    let (module, msg) = match rest.split_once(char::is_whitespace) {
        Some(p) => p,
        None => (rest, ""),
    };
    let module = module.strip_suffix(':')?;
    if module.is_empty() {
        return None;
    }
    Some(Entry {
        level: level.to_uppercase(),
        module: module.to_string(),
        message: msg.trim().to_string(),
    })
}

fn parse_all(text: &str) -> (Vec<Entry>, Vec<usize>) {
    let mut entries = Vec::new();
    let mut bad = Vec::new();
    for (i, line) in text.lines().enumerate() {
        if line.trim().is_empty() {
            continue;
        }
        match parse_line(line) {
            Some(e) => entries.push(e),
            None => bad.push(i + 1),
        }
    }
    (entries, bad)
}

fn count_levels(entries: &[Entry]) -> BTreeMap<String, usize> {
    let mut m = BTreeMap::new();
    for e in entries {
        *m.entry(e.level.clone()).or_insert(0) += 1;
    }
    m
}

fn busiest_module(entries: &[Entry]) -> Option<(String, usize)> {
    let mut m: BTreeMap<String, usize> = BTreeMap::new();
    for e in entries {
        *m.entry(e.module.clone()).or_insert(0) += 1;
    }
    m.into_iter()
        .min_by_key(|(name, c)| (std::cmp::Reverse(*c), name.clone()))
}

fn messages_for(entries: &[Entry], level: &str) -> Vec<String> {
    entries
        .iter()
        .filter(|e| e.level.eq_ignore_ascii_case(level) && !e.message.is_empty())
        .map(|e| e.message.clone())
        .collect()
}

fn main() {
    println!("{}", parse_all("INFO a: b").0.len());
}
