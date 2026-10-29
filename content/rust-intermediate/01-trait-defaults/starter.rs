trait Scorer {
    const NAME: &'static str;
    fn score(&self, word: &str) -> u32;

    fn total(&self, words: &[&str]) -> u32 {
        0
    }

    fn best(&self, words: &[&str]) -> Option<usize> {
        None
    }

    fn label(&self) -> String {
        String::new()
    }
}

struct LengthScorer;
struct VowelScorer;
struct WeightedScorer {
    weight: u32,
}

impl Scorer for LengthScorer {
    const NAME: &'static str = "length";
    fn score(&self, word: &str) -> u32 {
        0
    }
}

impl Scorer for VowelScorer {
    const NAME: &'static str = "vowel";
    fn score(&self, word: &str) -> u32 {
        0
    }
}

impl Scorer for WeightedScorer {
    const NAME: &'static str = "weighted";
    fn score(&self, word: &str) -> u32 {
        0
    }
}

// Try it with Run.
fn main() {
    let words = ["cat", "horse", "education"];
    println!("{} {}", LengthScorer.label(), LengthScorer.total(&words));
    println!("{:?}", VowelScorer.best(&words));
}
