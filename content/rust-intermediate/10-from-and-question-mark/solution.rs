use std::fmt;
use std::num::{ParseFloatError, ParseIntError};

#[derive(Debug, PartialEq)]
enum AppError {
    BadInt(ParseIntError),
    BadFloat(ParseFloatError),
    Negative(i64),
}

impl From<ParseIntError> for AppError {
    fn from(e: ParseIntError) -> AppError {
        AppError::BadInt(e)
    }
}

impl From<ParseFloatError> for AppError {
    fn from(e: ParseFloatError) -> AppError {
        AppError::BadFloat(e)
    }
}

impl fmt::Display for AppError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            AppError::BadInt(e) => write!(f, "bad integer: {e}"),
            AppError::BadFloat(e) => write!(f, "bad float: {e}"),
            AppError::Negative(n) => write!(f, "negative value: {n}"),
        }
    }
}

fn parse_scaled(count: &str, factor: &str) -> Result<f64, AppError> {
    let n: i64 = count.trim().parse()?;
    if n < 0 {
        return Err(AppError::Negative(n));
    }
    let k: f64 = factor.trim().parse()?;
    Ok(n as f64 * k)
}

fn total_scaled(pairs: &[(&str, &str)]) -> Result<f64, AppError> {
    let mut total = 0.0;
    for (a, b) in pairs {
        total += parse_scaled(a, b)?;
    }
    Ok(total)
}

fn collect_valid(pairs: &[(&str, &str)]) -> (Vec<f64>, Vec<String>) {
    let mut values = Vec::new();
    let mut errors = Vec::new();
    for (a, b) in pairs {
        match parse_scaled(a, b) {
            Ok(v) => values.push(v),
            Err(e) => errors.push(e.to_string()),
        }
    }
    (values, errors)
}

// Try it with Run.
fn main() {
    println!("{:?}", parse_scaled("4", "2.5"));
    println!("{:?}", collect_valid(&[("3", "1.5"), ("x", "1"), ("-2", "1")]));
}
