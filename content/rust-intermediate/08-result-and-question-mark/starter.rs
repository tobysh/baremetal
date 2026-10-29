use std::num::ParseIntError;

fn parse_sum(a: &str, b: &str) -> Result<i32, ParseIntError> {
    Ok(0)
}

fn sum_all(items: &[&str]) -> Result<i32, ParseIntError> {
    Ok(0)
}

fn first_even(items: &[&str]) -> Result<Option<i32>, ParseIntError> {
    Ok(None)
}

fn parse_or(s: &str, default: i32) -> i32 {
    0
}

fn describe(s: &str) -> String {
    String::new()
}

// Try it with Run.
fn main() {
    println!("{:?}", parse_sum("20", " 22 "));
    println!("{:?}", first_even(&["3", "7", "10", "x"]));
    println!("{}", describe("abc"));
}
