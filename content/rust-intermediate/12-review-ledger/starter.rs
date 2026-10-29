use std::collections::BTreeMap;
use std::error::Error;
use std::fmt;
use std::num::ParseIntError;

#[derive(Debug, PartialEq)]
enum LedgerError {
    Syntax(usize),
    Amount { line: usize, source: ParseIntError },
    Overdraft { name: String, balance: i64 },
}

impl fmt::Display for LedgerError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        Ok(())
    }
}

impl Error for LedgerError {}

fn apply(text: &str) -> Result<BTreeMap<String, i64>, LedgerError> {
    Ok(BTreeMap::new())
}

fn report(text: &str) -> String {
    String::new()
}

// Try it with Run.
fn main() {
    println!("{}", report("alice:+50\nbob:+20\nalice:-30"));
    println!("{}", report("alice:+10\nalice:-15"));
}
