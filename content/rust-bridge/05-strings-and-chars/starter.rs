fn is_palindrome(s: &str) -> bool {
    false
}

fn title_case(s: &str) -> String {
    String::new()
}

fn truncate_chars(s: &str, max: usize) -> String {
    String::new()
}

fn caesar(s: &str, shift: i32) -> String {
    String::new()
}

fn split_key_value(line: &str) -> Option<(&str, &str)> {
    None
}

// Try it with Run.
fn main() {
    println!("{}", is_palindrome("A man, a plan, a canal: Panama"));
    println!("{}", title_case("  hELLO   wORLD "));
    println!("{}", truncate_chars("héllo wörld", 4));
    println!("{}", caesar("Hello, World!", 3));
    println!("{:?}", split_key_value(" name = Ada "));
}
