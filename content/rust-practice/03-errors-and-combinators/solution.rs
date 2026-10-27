use std::fmt;
use std::num::ParseIntError;

#[derive(Debug, PartialEq)]
enum PortError {
    Missing,
    Empty,
    NotANumber(ParseIntError),
    OutOfRange(i64),
}

impl fmt::Display for PortError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            PortError::Missing => write!(f, "no port configured"),
            PortError::Empty => write!(f, "port is empty"),
            PortError::NotANumber(e) => write!(f, "not a number: {}", e),
            PortError::OutOfRange(n) => write!(f, "port {} is out of range (1-65535)", n),
        }
    }
}

impl std::error::Error for PortError {}

impl From<ParseIntError> for PortError {
    fn from(e: ParseIntError) -> PortError {
        PortError::NotANumber(e)
    }
}

fn parse_port(s: &str) -> Result<u16, PortError> {
    let trimmed = s.trim();
    if trimmed.is_empty() {
        return Err(PortError::Empty);
    }
    let n: i64 = trimmed.parse()?;
    if !(1..=65535).contains(&n) {
        return Err(PortError::OutOfRange(n));
    }
    Ok(n as u16)
}

fn lookup<'a>(config: &'a str, key: &str) -> Option<&'a str> {
    config
        .lines()
        .filter_map(|line| line.split_once('='))
        .find(|(k, _)| k.trim() == key)
        .map(|(_, v)| v.trim())
}

fn port_from_config(config: &str) -> Result<u16, PortError> {
    let raw = lookup(config, "port").ok_or(PortError::Missing)?;
    parse_port(raw)
}

fn halve_even(n: Option<i32>) -> Option<i32> {
    n.filter(|x| x % 2 == 0).map(|x| x / 2)
}

// Try it with Run.
fn main() {
    println!("{:?}", parse_port("8080"));
    match port_from_config("host=a\nport=99999") {
        Ok(p) => println!("port {}", p),
        Err(e) => println!("error: {}", e),
    }
    println!("{:?}", halve_even(Some(10)));
}
