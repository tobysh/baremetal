fn close(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-6
}

#[test]
fn circle_area() {
    let r = area(&Shape::Circle(2.0));
    assert!(close(r, 12.566370614359172), "area of Circle(2.0) should be about 12.566371, got {}", r);
}

#[test]
fn circle_perimeter() {
    let r = perimeter(&Shape::Circle(1.0));
    assert!(close(r, 6.283185307179586), "perimeter of Circle(1.0) should be about 6.283185, got {}", r);
}

#[test]
fn rectangle_area() {
    let r = area(&Shape::Rectangle(3.0, 4.5));
    assert!(close(r, 13.5), "area of Rectangle(3.0, 4.5) should be 13.5, got {}", r);
}

#[test]
fn rectangle_perimeter() {
    let r = perimeter(&Shape::Rectangle(3.0, 4.5));
    assert!(close(r, 15.0), "perimeter of Rectangle(3.0, 4.5) should be 15.0, got {}", r);
}

#[test]
fn right_triangle_area() {
    let r = area(&Shape::Triangle(3.0, 4.0, 5.0));
    assert!(close(r, 6.0), "area of Triangle(3.0, 4.0, 5.0) should be 6.0, got {}", r);
}

#[test]
fn equilateral_triangle_area() {
    let r = area(&Shape::Triangle(2.0, 2.0, 2.0));
    assert!(close(r, 3.0_f64.sqrt()), "area of Triangle(2.0, 2.0, 2.0) should be about 1.732051, got {}", r);
}

#[test]
fn triangle_perimeter() {
    let r = perimeter(&Shape::Triangle(3.0, 4.0, 5.0));
    assert!(close(r, 12.0), "perimeter of Triangle(3.0, 4.0, 5.0) should be 12.0, got {}", r);
}

#[test]
fn zero_radius_circle_has_zero_area() {
    let r = area(&Shape::Circle(0.0));
    assert!(close(r, 0.0), "area of Circle(0.0) should be 0.0, got {}", r);
}
