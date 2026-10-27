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
        0.0
    }
    fn name(&self) -> String {
        String::new()
    }
}

impl Shape for Rect {
    fn area(&self) -> f64 {
        0.0
    }
    fn name(&self) -> String {
        String::new()
    }
}

impl Shape for Circle {
    fn area(&self) -> f64 {
        0.0
    }
    fn name(&self) -> String {
        String::new()
    }
}

fn total_area<T: Shape>(items: &[T]) -> f64 {
    0.0
}

fn describe_largest(items: &[Box<dyn Shape>]) -> Option<String> {
    None
}

fn report(items: &[Box<dyn Shape>]) -> String {
    String::new()
}

// Try your shapes with Run.
fn main() {
    let shapes: Vec<Box<dyn Shape>> = vec![Box::new(Square(2.0)), Box::new(Circle { r: 1.0 })];
    println!("{}", report(&shapes));
    println!("{:?}", describe_largest(&shapes));
    let _ = PI;
}
