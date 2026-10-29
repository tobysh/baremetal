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
        AppError::Negative(0)
    }
}

impl From<ParseFloatError> for AppError {
    fn from(e: ParseFloatError) -> AppError {
        AppError::Negative(0)
    }
}

impl fmt::Display for AppError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        Ok(())
    }
}

fn parse_scaled(count: &str, factor: &str) -> Result<f64, AppError> {
    Ok(0.0)
}

fn total_scaled(pairs: &[(&str, &str)]) -> Result<f64, AppError> {
    Ok(0.0)
}

fn collect_valid(pairs: &[(&str, &str)]) -> (Vec<f64>, Vec<String>) {
    (Vec::new(), Vec::new())
}

// Try it with Run.
fn main() {
    println!("{:?}", parse_scaled("4", "2.5"));
    println!("{:?}", collect_valid(&[("3", "1.5"), ("x", "1"), ("-2", "1")]));
}
