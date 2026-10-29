use std::collections::BTreeMap;
use std::fmt;
use std::sync::{mpsc, Arc, Mutex};
use std::thread;

#[derive(Debug, PartialEq)]
enum LogError {
    BadLine(usize),
}

impl fmt::Display for LogError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        Ok(())
    }
}

fn level_of(line: &str, number: usize) -> Result<&str, LogError> {
    Err(LogError::BadLine(number))
}

fn summarize(lines: Vec<String>, workers: usize) -> Result<BTreeMap<String, usize>, LogError> {
    Ok(BTreeMap::new())
}

// Try it with Run.
fn main() {
    let lines = vec!["INFO started".to_string(), "WARN disk".to_string(), "INFO ok".to_string()];
    println!("{:?}", summarize(lines, 2));
}
