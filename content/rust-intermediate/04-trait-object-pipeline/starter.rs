trait Stage {
    fn name(&self) -> String;
    fn apply(&self, x: i64) -> Option<i64>;
}

struct AddN(i64);
struct MulN(i64);
struct NonNeg;
struct Clamp(i64);

impl Stage for AddN {
    fn name(&self) -> String {
        String::new()
    }
    fn apply(&self, x: i64) -> Option<i64> {
        None
    }
}

impl Stage for MulN {
    fn name(&self) -> String {
        String::new()
    }
    fn apply(&self, x: i64) -> Option<i64> {
        None
    }
}

impl Stage for NonNeg {
    fn name(&self) -> String {
        String::new()
    }
    fn apply(&self, x: i64) -> Option<i64> {
        None
    }
}

impl Stage for Clamp {
    fn name(&self) -> String {
        String::new()
    }
    fn apply(&self, x: i64) -> Option<i64> {
        None
    }
}

fn build(spec: &str) -> Option<Box<dyn Stage>> {
    None
}

fn build_all(specs: &[&str]) -> Option<Vec<Box<dyn Stage>>> {
    None
}

fn run(stages: &[Box<dyn Stage>], x: i64) -> Option<i64> {
    None
}

fn names(stages: &[Box<dyn Stage>]) -> Vec<String> {
    Vec::new()
}

// Try it with Run.
fn main() {
    let stages = build_all(&["add:5", "mul:3", "clamp:30"]).unwrap();
    println!("{:?} {:?}", names(&stages), run(&stages, 7));
}
