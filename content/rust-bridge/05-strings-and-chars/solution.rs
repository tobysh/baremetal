fn is_palindrome(s: &str) -> bool {
    let c: Vec<char> = s
        .chars()
        .filter(|c| c.is_alphanumeric())
        .flat_map(|c| c.to_lowercase())
        .collect();
    c.iter().eq(c.iter().rev())
}

fn title_case(s: &str) -> String {
    let words: Vec<String> = s
        .split_whitespace()
        .map(|w| {
            let mut cs = w.chars();
            match cs.next() {
                Some(f) => f.to_uppercase().collect::<String>() + &cs.as_str().to_lowercase(),
                None => String::new(),
            }
        })
        .collect();
    words.join(" ")
}

fn truncate_chars(s: &str, max: usize) -> String {
    s.chars().take(max).collect()
}

fn caesar(s: &str, shift: i32) -> String {
    s.chars()
        .map(|c| {
            if c.is_ascii_alphabetic() {
                let base = if c.is_ascii_lowercase() { b'a' } else { b'A' };
                let off = ((c as u8 - base) as i32 + shift).rem_euclid(26) as u8;
                (base + off) as char
            } else {
                c
            }
        })
        .collect()
}

fn split_key_value(line: &str) -> Option<(&str, &str)> {
    let (k, v) = line.split_once('=')?;
    let k = k.trim();
    if k.is_empty() {
        None
    } else {
        Some((k, v.trim()))
    }
}

fn main() {
    println!("{}", caesar("Hello, World!", 3));
}
