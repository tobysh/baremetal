fn run_lengths(s: &str) -> Vec<(char, usize)> {
    let mut out = Vec::new();
    let mut it = s.chars().peekable();
    while let Some(c) = it.next() {
        let mut n = 1;
        while it.peek() == Some(&c) {
            it.next();
            n += 1;
        }
        out.push((c, n));
    }
    out
}

fn all_words(lines: &[&str]) -> Vec<String> {
    lines
        .iter()
        .flat_map(|l| l.split_whitespace())
        .map(|w| w.to_lowercase())
        .collect()
}

fn split_by_sign(v: &[i32]) -> (Vec<i32>, Vec<i32>) {
    v.iter().copied().partition(|&x| x >= 0)
}

fn first_big_square(v: &[i32], limit: i32) -> Option<i32> {
    v.iter().find_map(|&x| {
        let sq = x * x;
        if sq > limit { Some(sq) } else { None }
    })
}

fn numbers_in(items: &[&str]) -> Vec<i32> {
    items.iter().filter_map(|s| s.parse().ok()).collect()
}

fn main() {
    println!("{:?}", run_lengths("aaabcc"));
}
