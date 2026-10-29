use std::num::ParseIntError;

fn parse_sum(a: &str, b: &str) -> Result<i32, ParseIntError> {
    Ok(a.trim().parse::<i32>()? + b.trim().parse::<i32>()?)
}

fn sum_all(items: &[&str]) -> Result<i32, ParseIntError> {
    let mut total = 0;
    for s in items {
        total += s.trim().parse::<i32>()?;
    }
    Ok(total)
}

fn first_even(items: &[&str]) -> Result<Option<i32>, ParseIntError> {
    for s in items {
        let n: i32 = s.trim().parse()?;
        if n % 2 == 0 {
            return Ok(Some(n));
        }
    }
    Ok(None)
}

fn parse_or(s: &str, default: i32) -> i32 {
    s.trim().parse().unwrap_or(default)
}

fn describe(s: &str) -> String {
    match s.trim().parse::<i32>() {
        Ok(n) => format!("number {n}"),
        Err(e) => format!("error: {e}"),
    }
}

// Try it with Run.
fn main() {
    println!("{:?}", parse_sum("20", " 22 "));
    println!("{:?}", first_even(&["3", "7", "10", "x"]));
    println!("{}", describe("abc"));
}
