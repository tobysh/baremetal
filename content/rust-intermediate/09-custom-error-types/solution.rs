use std::error::Error;
use std::fmt;

#[derive(Debug, PartialEq)]
enum ConfigError {
    NoSettings,
    MissingEquals(usize),
    BadNumber { line: usize, text: String },
    Duplicate(String),
}

impl fmt::Display for ConfigError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            ConfigError::NoSettings => write!(f, "no settings found"),
            ConfigError::MissingEquals(n) => write!(f, "line {n}: expected key = value"),
            ConfigError::BadNumber { line, text } => write!(f, "line {line}: '{text}' is not a number"),
            ConfigError::Duplicate(k) => write!(f, "duplicate key '{k}'"),
        }
    }
}

impl Error for ConfigError {}

fn parse_config(text: &str) -> Result<Vec<(String, i64)>, ConfigError> {
    let mut out: Vec<(String, i64)> = Vec::new();
    for (i, raw) in text.lines().enumerate() {
        let n = i + 1;
        let line = raw.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let (k, v) = line.split_once('=').ok_or(ConfigError::MissingEquals(n))?;
        let key = k.trim();
        let val = v.trim();
        let num: i64 = val.parse().map_err(|_| ConfigError::BadNumber { line: n, text: val.to_string() })?;
        if out.iter().any(|(existing, _)| existing == key) {
            return Err(ConfigError::Duplicate(key.to_string()));
        }
        out.push((key.to_string(), num));
    }
    if out.is_empty() {
        Err(ConfigError::NoSettings)
    } else {
        Ok(out)
    }
}

// Try it with Run.
fn main() {
    match parse_config("width = 80\nheight = x") {
        Ok(v) => println!("{:?}", v),
        Err(e) => println!("error: {}", e),
    }
}
