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
        match self {
            LedgerError::Syntax(n) => write!(f, "line {n}: expected name:amount"),
            LedgerError::Amount { line, source } => write!(f, "line {line}: bad amount ({source})"),
            LedgerError::Overdraft { name, balance } => write!(f, "{name} would go to {balance}"),
        }
    }
}

impl Error for LedgerError {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        match self {
            LedgerError::Amount { source, .. } => Some(source),
            _ => None,
        }
    }
}

fn apply(text: &str) -> Result<BTreeMap<String, i64>, LedgerError> {
    let mut map = BTreeMap::new();
    for (i, raw) in text.lines().enumerate() {
        let n = i + 1;
        let line = raw.trim();
        if line.is_empty() {
            continue;
        }
        let (name, amt) = line.split_once(':').ok_or(LedgerError::Syntax(n))?;
        let name = name.trim();
        if name.is_empty() {
            return Err(LedgerError::Syntax(n));
        }
        let amount: i64 = amt.trim().parse().map_err(|e| LedgerError::Amount { line: n, source: e })?;
        let bal = map.entry(name.to_string()).or_insert(0);
        if *bal + amount < 0 {
            return Err(LedgerError::Overdraft { name: name.to_string(), balance: *bal + amount });
        }
        *bal += amount;
    }
    Ok(map)
}

fn report(text: &str) -> String {
    match apply(text) {
        Ok(m) => m.iter().map(|(k, v)| format!("{k}={v}")).collect::<Vec<_>>().join(", "),
        Err(e) => format!("error: {e}"),
    }
}

// Try it with Run.
fn main() {
    println!("{}", report("alice:+50\nbob:+20\nalice:-30"));
    println!("{}", report("alice:+10\nalice:-15"));
}
