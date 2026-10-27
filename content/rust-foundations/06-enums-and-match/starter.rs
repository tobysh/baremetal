use std::f64::consts::PI;

enum Shape {
    Circle(f64),
    Rectangle(f64, f64),
    Triangle(f64, f64, f64),
}

fn area(s: &Shape) -> f64 {
    0.0
}

fn perimeter(s: &Shape) -> f64 {
    0.0
}

// Try your functions with Run.
fn main() {
    let shapes = [Shape::Circle(1.0), Shape::Rectangle(2.0, 3.0), Shape::Triangle(3.0, 4.0, 5.0)];
    for s in &shapes {
        println!("area {:.2}, perimeter {:.2}", area(s), perimeter(s));
    }
}
