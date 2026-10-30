fn parse_sum(input: &str) -> Result<i64, String> {
    if input.trim().is_empty() {
        return Ok(0);
    }
    let mut total = 0;
    for piece in input.split(',') {
        let item = piece.trim();
        total += item
            .parse::<i64>()
            .map_err(|_| format!("invalid number: {}", item))?;
    }
    Ok(total)
}

fn main() {
    println!("{:?}", parse_sum("1, 2, 3"));
    println!("{:?}", parse_sum("4, abc"));
}
