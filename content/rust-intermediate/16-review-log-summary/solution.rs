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
        match self {
            LogError::BadLine(n) => write!(f, "line {n}: unknown level"),
        }
    }
}

fn level_of(line: &str, number: usize) -> Result<&str, LogError> {
    let word = line.split_whitespace().next().unwrap_or("");
    match word {
        "INFO" | "WARN" | "ERROR" => Ok(word),
        _ => Err(LogError::BadLine(number)),
    }
}

fn summarize(lines: Vec<String>, workers: usize) -> Result<BTreeMap<String, usize>, LogError> {
    let workers = workers.max(1);
    let size = ((lines.len() + workers - 1) / workers).max(1);
    let counts = Arc::new(Mutex::new(BTreeMap::new()));
    let (tx, rx) = mpsc::channel();
    let mut handles = Vec::new();
    for (ci, chunk) in lines.chunks(size).enumerate() {
        let chunk = chunk.to_vec();
        let counts = Arc::clone(&counts);
        let tx = tx.clone();
        let first = ci * size + 1;
        handles.push(thread::spawn(move || {
            for (j, line) in chunk.iter().enumerate() {
                match level_of(line, first + j) {
                    Ok(level) => {
                        *counts.lock().unwrap().entry(level.to_string()).or_insert(0) += 1;
                    }
                    Err(LogError::BadLine(n)) => tx.send(n).unwrap(),
                }
            }
        }));
    }
    drop(tx);
    for h in handles {
        h.join().unwrap();
    }
    if let Some(n) = rx.iter().min() {
        return Err(LogError::BadLine(n));
    }
    let map = counts.lock().unwrap().clone();
    Ok(map)
}

// Try it with Run.
fn main() {
    let lines = vec!["INFO started".to_string(), "WARN disk".to_string(), "INFO ok".to_string()];
    println!("{:?}", summarize(lines, 2));
}
