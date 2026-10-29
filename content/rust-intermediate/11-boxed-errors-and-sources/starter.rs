use std::error::Error;
use std::fmt;
use std::num::ParseIntError;

#[derive(Debug)]
struct LoadError {
    name: String,
    source: ParseIntError,
}

impl fmt::Display for LoadError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "could not load '{}'", self.name)
    }
}

impl Error for LoadError {}

fn read_pair(s: &str) -> Result<(i32, i32), Box<dyn Error>> {
    Ok((0, 0))
}

fn load(name: &str, text: &str) -> Result<i32, LoadError> {
    Ok(0)
}

fn load_all(entries: &[(&str, &str)]) -> Result<Vec<i32>, Box<dyn Error>> {
    Ok(Vec::new())
}

fn chain(e: &dyn Error) -> Vec<String> {
    Vec::new()
}

fn is_int_error(e: &(dyn Error + 'static)) -> bool {
    false
}

// Try it with Run.
fn main() {
    match load_all(&[("a", "1"), ("b", "two")]) {
        Ok(v) => println!("{:?}", v),
        Err(e) => println!("{:?}", chain(e.as_ref())),
    }
}
