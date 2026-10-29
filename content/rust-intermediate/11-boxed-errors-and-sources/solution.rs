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

impl Error for LoadError {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        Some(&self.source)
    }
}

fn read_pair(s: &str) -> Result<(i32, i32), Box<dyn Error>> {
    let (a, b) = s.split_once(',').ok_or("expected 'a,b'")?;
    Ok((a.trim().parse()?, b.trim().parse()?))
}

fn load(name: &str, text: &str) -> Result<i32, LoadError> {
    text.trim().parse().map_err(|e| LoadError { name: name.to_string(), source: e })
}

fn load_all(entries: &[(&str, &str)]) -> Result<Vec<i32>, Box<dyn Error>> {
    let mut out = Vec::new();
    for (name, text) in entries {
        out.push(load(name, text)?);
    }
    Ok(out)
}

fn chain(e: &dyn Error) -> Vec<String> {
    let mut out = vec![e.to_string()];
    let mut cur = e.source();
    while let Some(s) = cur {
        out.push(s.to_string());
        cur = s.source();
    }
    out
}

fn is_int_error(e: &(dyn Error + 'static)) -> bool {
    e.downcast_ref::<ParseIntError>().is_some()
}

// Try it with Run.
fn main() {
    match load_all(&[("a", "1"), ("b", "two")]) {
        Ok(v) => println!("{:?}", v),
        Err(e) => println!("{:?}", chain(e.as_ref())),
    }
}
