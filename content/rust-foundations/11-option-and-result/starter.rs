fn find_index(v: &[i32], target: i32) -> Option<usize> {
    None
}

fn first_even(v: &[i32]) -> Option<i32> {
    None
}

fn describe(o: Option<i32>) -> String {
    String::new()
}

fn double_or_zero(o: Option<i32>) -> i32 {
    -1
}

fn safe_div(a: i32, b: i32) -> Result<i32, String> {
    Err(String::new())
}

fn parse_age(s: &str) -> Result<u8, String> {
    Err(String::new())
}

// Try your functions with Run.
fn main() {
    println!("{:?}", find_index(&[5, 6, 7], 6));
    println!("{}", describe(Some(3)));
    println!("{:?}", safe_div(7, 2));
    println!("{:?}", parse_age("300"));
}
