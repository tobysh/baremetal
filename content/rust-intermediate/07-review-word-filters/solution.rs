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
        word.starts_with(self.0)
    }
}

impl Matcher for MinLen {
    fn matches(&self, word: &str) -> bool {
        word.len() >= self.0
    }
}

fn select<'a, M: Matcher>(text: &'a str, m: &M) -> Vec<&'a str> {
    text.split_whitespace().filter(|w| m.matches(w)).collect()
}

fn select_all<'a>(text: &'a str, ms: &[&dyn Matcher]) -> Vec<&'a str> {
    text.split_whitespace().filter(|w| ms.iter().all(|m| m.matches(w))).collect()
}

fn first_match<'a>(text: &'a str, m: &dyn Matcher) -> Option<&'a str> {
    text.split_whitespace().find(|w| m.matches(w))
}

// Try it with Run.
fn main() {
    let text = "the cat sat on a mat near the cart";
    println!("{:?}", select(text, &Prefix("ca")));
    println!("{:?}", select_all(text, &[&MinLen(3), &|w: &str| w.ends_with('t')]));
}
