use std::collections::BTreeMap;

struct Entry {
    level: String,
    module: String,
    message: String,
}

fn parse_line(line: &str) -> Option<Entry> {
    None
}

fn parse_all(text: &str) -> (Vec<Entry>, Vec<usize>) {
    (Vec::new(), Vec::new())
}

fn count_levels(entries: &[Entry]) -> BTreeMap<String, usize> {
    BTreeMap::new()
}

fn busiest_module(entries: &[Entry]) -> Option<(String, usize)> {
    None
}

fn messages_for(entries: &[Entry], level: &str) -> Vec<String> {
    Vec::new()
}

// Try it with Run.
fn main() {
    let text = "ERROR net: connection lost\n\nwarn disk:  almost full  \nbroken line\nINFO net:\n";
    let (entries, bad) = parse_all(text);
    println!("{} entries, bad lines {:?}", entries.len(), bad);
    println!("{:?}", count_levels(&entries));
    println!("{:?}", busiest_module(&entries));
    println!("{:?}", messages_for(&entries, "warn"));
    if let Some(e) = parse_line("ERROR net: x") {
        println!("{} {} {}", e.level, e.module, e.message);
    }
}
