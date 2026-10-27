fn parse_sum(input: &str) -> Result<i64, String> {
    Err(String::new())
}

// Try your function with Run.
fn main() {
    println!("{:?}", parse_sum("1, 2, 3"));
    println!("{:?}", parse_sum("4, abc"));
}
