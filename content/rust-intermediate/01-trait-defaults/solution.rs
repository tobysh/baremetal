trait Scorer {
    const NAME: &'static str;
    fn score(&self, word: &str) -> u32;

    fn total(&self, words: &[&str]) -> u32 {
        words.iter().map(|w| self.score(w)).sum()
    }

    fn best(&self, words: &[&str]) -> Option<usize> {
        let mut best: Option<(usize, u32)> = None;
        for (i, w) in words.iter().enumerate() {
            let s = self.score(w);
            match best {
                Some((_, b)) if b >= s => {}
                _ => best = Some((i, s)),
            }
        }
        best.map(|(i, _)| i)
    }

    fn label(&self) -> String {
        format!("scorer:{}", Self::NAME)
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
        word.len() as u32
    }
}

impl Scorer for VowelScorer {
    const NAME: &'static str = "vowel";
    fn score(&self, word: &str) -> u32 {
        word.chars().filter(|c| "aeiouAEIOU".contains(*c)).count() as u32
    }
}

impl Scorer for WeightedScorer {
    const NAME: &'static str = "weighted";
    fn score(&self, word: &str) -> u32 {
        word.len() as u32 * self.weight
    }
}

// Try it with Run.
fn main() {
    let words = ["cat", "horse", "education"];
    println!("{} {}", LengthScorer.label(), LengthScorer.total(&words));
    println!("{:?}", VowelScorer.best(&words));
}
