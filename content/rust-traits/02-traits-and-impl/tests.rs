#[test]
fn square_area_and_perimeter() {
    let s = Square { side: 4 };
    assert_eq!(s.area(), 16, "square of side 4 has area 16");
    assert_eq!(s.perimeter(), 16, "square of side 4 has perimeter 16");
}

#[test]
fn rectangle_area_and_perimeter() {
    let r = Rectangle { width: 3, height: 5 };
    assert_eq!(r.area(), 15, "3 by 5 rectangle has area 15");
    assert_eq!(r.perimeter(), 16, "3 by 5 rectangle has perimeter 16");
}

#[test]
fn names() {
    assert_eq!(Square { side: 1 }.name(), "square", "name of a square");
    assert_eq!(Rectangle { width: 1, height: 2 }.name(), "rectangle", "name of a rectangle");
}

#[test]
fn default_describe_uses_the_other_methods() {
    let s = Square { side: 3 };
    assert_eq!(s.describe(), "square: area 9, perimeter 12", "describe for a square of side 3");
    let r = Rectangle { width: 2, height: 6 };
    assert_eq!(r.describe(), "rectangle: area 12, perimeter 16", "describe for a 2 by 6 rectangle");
}

#[test]
fn total_area_of_squares() {
    let v = vec![Square { side: 1 }, Square { side: 2 }, Square { side: 3 }];
    assert_eq!(total_area(&v), 14, "1 + 4 + 9 = 14");
}

#[test]
fn total_area_of_rectangles() {
    let v = vec![Rectangle { width: 2, height: 3 }, Rectangle { width: 4, height: 5 }];
    assert_eq!(total_area(&v), 26, "6 + 20 = 26");
}

#[test]
fn total_area_of_nothing_is_zero() {
    let v: Vec<Square> = Vec::new();
    assert_eq!(total_area(&v), 0, "no shapes, no area");
}
