fn find_index(v: &[i32], target: i32) -> Option<usize> {
    for (i, &x) in v.iter().enumerate() {
        if x == target {
            return Some(i);
        }
    }
    None
}

fn first_even(v: &[i32]) -> Option<i32> {
    v.iter().copied().find(|x| x % 2 == 0)
}

fn describe(o: Option<i32>) -> String {
    match o {
        Some(n) => format!("got {}", n),
        None => "none".to_string(),
    }
}

fn double_or_zero(o: Option<i32>) -> i32 {
    o.map(|x| x * 2).unwrap_or(0)
}

fn safe_div(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        Err("division by zero".to_string())
    } else {
        Ok(a / b)
    }
}

fn parse_age(s: &str) -> Result<u8, String> {
    s.parse::<u8>().map_err(|_| "not a valid age".to_string())
}

fn main() {
    println!("{:?}", find_index(&[5, 6, 7], 6));
}
