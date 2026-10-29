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
        Ok(())
    }
}

impl Error for ConfigError {}

fn parse_config(text: &str) -> Result<Vec<(String, i64)>, ConfigError> {
    Err(ConfigError::NoSettings)
}

// Try it with Run.
fn main() {
    match parse_config("width = 80\nheight = x") {
        Ok(v) => println!("{:?}", v),
        Err(e) => println!("error: {}", e),
    }
}
