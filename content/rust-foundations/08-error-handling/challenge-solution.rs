fn parse_report(input: &str) -> Result<i64, Vec<String>> {
    if input.trim().is_empty() {
        return Ok(0);
    }
    let mut problems = Vec::new();
    let mut values = Vec::new();
    for (i, piece) in input.split(',').enumerate() {
        let item = piece.trim();
        let n = i + 1;
        if item.is_empty() {
            problems.push(format!("item {} is empty", n));
            continue;
        }
        match item.parse::<i64>() {
            Ok(v) => values.push(v),
            Err(_) => problems.push(format!("item {} is not a number: {}", n, item)),
        }
    }
    if !problems.is_empty() {
        return Err(problems);
    }
    let mut total: i64 = 0;
    for v in values {
        total = match total.checked_add(v) {
            Some(t) => t,
            None => return Err(vec!["sum overflows".to_string()]),
        };
    }
    Ok(total)
}

fn main() {
    println!("{:?}", parse_report("1, x, , 4"));
    println!("{:?}", parse_report("1, 2, 3"));
}
