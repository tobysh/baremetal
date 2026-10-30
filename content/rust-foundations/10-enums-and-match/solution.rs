use std::f64::consts::PI;

enum Shape {
    Circle(f64),
    Rectangle(f64, f64),
    Triangle(f64, f64, f64),
}

fn area(s: &Shape) -> f64 {
    match s {
        Shape::Circle(r) => PI * r * r,
        Shape::Rectangle(w, h) => w * h,
        Shape::Triangle(a, b, c) => {
            let p = (a + b + c) / 2.0;
            (p * (p - a) * (p - b) * (p - c)).sqrt()
        }
    }
}

fn perimeter(s: &Shape) -> f64 {
    match s {
        Shape::Circle(r) => 2.0 * PI * r,
        Shape::Rectangle(w, h) => 2.0 * (w + h),
        Shape::Triangle(a, b, c) => a + b + c,
    }
}

fn main() {
    let shapes = [Shape::Circle(1.0), Shape::Rectangle(2.0, 3.0), Shape::Triangle(3.0, 4.0, 5.0)];
    for s in &shapes {
        println!("area {:.2}, perimeter {:.2}", area(s), perimeter(s));
    }
}
