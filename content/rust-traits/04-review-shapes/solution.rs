use std::f64::consts::PI;

trait Shape {
    fn area(&self) -> f64;
    fn name(&self) -> String;
    fn summary(&self) -> String {
        format!("{} (area {:.1})", self.name(), self.area())
    }
}

struct Square(f64);
struct Rect {
    w: f64,
    h: f64,
}
struct Circle {
    r: f64,
}

impl Shape for Square {
    fn area(&self) -> f64 {
        self.0 * self.0
    }
    fn name(&self) -> String {
        "square".to_string()
    }
}

impl Shape for Rect {
    fn area(&self) -> f64 {
        self.w * self.h
    }
    fn name(&self) -> String {
        "rect".to_string()
    }
}

impl Shape for Circle {
    fn area(&self) -> f64 {
        PI * self.r * self.r
    }
    fn name(&self) -> String {
        "circle".to_string()
    }
}

fn total_area<T: Shape>(items: &[T]) -> f64 {
    items.iter().map(|s| s.area()).sum()
}

fn describe_largest(items: &[Box<dyn Shape>]) -> Option<String> {
    items
        .iter()
        .max_by(|a, b| a.area().partial_cmp(&b.area()).unwrap())
        .map(|s| s.summary())
}

fn report(items: &[Box<dyn Shape>]) -> String {
    items.iter().map(|s| s.summary()).collect::<Vec<_>>().join("; ")
}

// Try your shapes with Run.
fn main() {
    let shapes: Vec<Box<dyn Shape>> = vec![Box::new(Square(2.0)), Box::new(Circle { r: 1.0 })];
    println!("{}", report(&shapes));
    println!("{:?}", describe_largest(&shapes));
}
