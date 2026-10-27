mod counter {
    pub struct Counter {
        count: u32,
        step: u32,
    }

    impl Counter {
        pub fn new(step: u32) -> Counter {
            Counter { count: 0, step: step.max(1) }
        }

        pub fn tick(&mut self) -> u32 {
            self.count = bump(self.count, self.step);
            self.count
        }

        pub fn count(&self) -> u32 {
            self.count
        }

        pub fn reset(&mut self) {
            self.count = 0;
        }
    }

    fn bump(count: u32, step: u32) -> u32 {
        count.saturating_add(step)
    }

    pub mod format {
        use super::Counter;

        pub fn label(c: &Counter) -> String {
            format!("count={}", c.count())
        }
    }
}

use counter::Counter;

// Try it with Run.
fn main() {
    let mut c = Counter::new(5);
    c.tick();
    c.tick();
    println!("{}", counter::format::label(&c));
}
