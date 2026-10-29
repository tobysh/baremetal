trait Matcher {
    fn matches(&self, word: &str) -> bool;
}

struct Prefix<'p>(&'p str);
struct MinLen(usize);

// Any closure from &str to bool is a Matcher. Already written for you.
impl<F: Fn(&str) -> bool> Matcher for F {
    fn matches(&self, word: &str) -> bool {
        self(word)
    }
}

impl Matcher for Prefix<'_> {
    fn matches(&self, word: &str) -> bool {
        false
    }
}

impl Matcher for MinLen {
    fn matches(&self, word: &str) -> bool {
        false
    }
}

fn select<'a, M: Matcher>(text: &'a str, m: &M) -> Vec<&'a str> {
    Vec::new()
}

fn select_all<'a>(text: &'a str, ms: &[&dyn Matcher]) -> Vec<&'a str> {
    Vec::new()
}

fn first_match<'a>(text: &'a str, m: &dyn Matcher) -> Option<&'a str> {
    None
}

// Try it with Run.
fn main() {
    let text = "the cat sat on a mat near the cart";
    println!("{:?}", select(text, &Prefix("ca")));
    println!("{:?}", select_all(text, &[&MinLen(3), &|w: &str| w.ends_with('t')]));
}
