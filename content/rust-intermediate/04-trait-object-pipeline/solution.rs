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
        format!("add:{}", self.0)
    }
    fn apply(&self, x: i64) -> Option<i64> {
        x.checked_add(self.0)
    }
}

impl Stage for MulN {
    fn name(&self) -> String {
        format!("mul:{}", self.0)
    }
    fn apply(&self, x: i64) -> Option<i64> {
        x.checked_mul(self.0)
    }
}

impl Stage for NonNeg {
    fn name(&self) -> String {
        "nonneg".to_string()
    }
    fn apply(&self, x: i64) -> Option<i64> {
        if x >= 0 {
            Some(x)
        } else {
            None
        }
    }
}

impl Stage for Clamp {
    fn name(&self) -> String {
        format!("clamp:{}", self.0)
    }
    fn apply(&self, x: i64) -> Option<i64> {
        Some(x.min(self.0))
    }
}

fn build(spec: &str) -> Option<Box<dyn Stage>> {
    let (kind, arg) = match spec.split_once(':') {
        Some((k, a)) => (k, Some(a)),
        None => (spec, None),
    };
    match (kind, arg) {
        ("add", Some(a)) => Some(Box::new(AddN(a.parse().ok()?))),
        ("mul", Some(a)) => Some(Box::new(MulN(a.parse().ok()?))),
        ("clamp", Some(a)) => Some(Box::new(Clamp(a.parse().ok()?))),
        ("nonneg", None) => Some(Box::new(NonNeg)),
        _ => None,
    }
}

fn build_all(specs: &[&str]) -> Option<Vec<Box<dyn Stage>>> {
    specs.iter().map(|s| build(s)).collect()
}

fn run(stages: &[Box<dyn Stage>], x: i64) -> Option<i64> {
    let mut v = x;
    for s in stages {
        v = s.apply(v)?;
    }
    Some(v)
}

fn names(stages: &[Box<dyn Stage>]) -> Vec<String> {
    stages.iter().map(|s| s.name()).collect()
}

// Try it with Run.
fn main() {
    let stages = build_all(&["add:5", "mul:3", "clamp:30"]).unwrap();
    println!("{:?} {:?}", names(&stages), run(&stages, 7));
}
