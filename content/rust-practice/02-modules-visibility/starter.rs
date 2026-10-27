mod counter {
    pub struct Counter {
        count: u32,
        step: u32,
    }

    impl Counter {
        pub fn new(step: u32) -> Counter {
            Counter { count: 0, step }
        }

        pub fn tick(&mut self) -> u32 {
            0
        }

        pub fn count(&self) -> u32 {
            self.count
        }

        pub fn reset(&mut self) {}
    }

    fn bump(count: u32, step: u32) -> u32 {
        count
    }

    pub mod format {
        use super::Counter;

        pub fn label(c: &Counter) -> String {
            String::new()
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
