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
        write!(f, "")
    }
}

impl std::error::Error for PortError {}

impl From<ParseIntError> for PortError {
    fn from(e: ParseIntError) -> PortError {
        PortError::Empty
    }
}

fn parse_port(s: &str) -> Result<u16, PortError> {
    Err(PortError::Empty)
}

fn lookup<'a>(config: &'a str, key: &str) -> Option<&'a str> {
    None
}

fn port_from_config(config: &str) -> Result<u16, PortError> {
    Err(PortError::Missing)
}

fn halve_even(n: Option<i32>) -> Option<i32> {
    None
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
