fn near(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-9
}

#[test]
fn square_area_and_name() {
    let s = Square(3.0);
    assert!(near(s.area(), 9.0), "Square(3.0).area() should be 9.0, got {}", s.area());
    assert_eq!(s.name(), "square", "Square's name should be \"square\"");
}

#[test]
fn rect_area_and_name() {
    let r = Rect { w: 2.0, h: 5.0 };
    assert!(near(r.area(), 10.0), "Rect 2x5 area should be 10.0, got {}", r.area());
    assert_eq!(r.name(), "rect", "Rect's name should be \"rect\"");
}

#[test]
fn circle_area_and_name() {
    let c = Circle { r: 2.0 };
    assert!(near(c.area(), 4.0 * std::f64::consts::PI), "Circle r=2 area should be 4*PI, got {}", c.area());
    assert_eq!(c.name(), "circle", "Circle's name should be \"circle\"");
}

#[test]
fn default_summary_is_inherited() {
    assert_eq!(Rect { w: 2.0, h: 3.0 }.summary(), "rect (area 6.0)", "summary should be \"rect (area 6.0)\"");
    assert_eq!(Circle { r: 1.0 }.summary(), "circle (area 3.1)", "summary should be \"circle (area 3.1)\"");
    assert_eq!(Square(2.0).summary(), "square (area 4.0)", "summary should be \"square (area 4.0)\"");
}

#[test]
fn total_area_of_one_type() {
    let v = vec![Square(2.0), Square(3.0)];
    assert!(near(total_area(&v), 13.0), "total_area of squares 2 and 3 should be 13.0, got {}", total_area(&v));
}

#[test]
fn total_area_works_for_other_types_and_empty() {
    let v = vec![Rect { w: 1.0, h: 2.0 }, Rect { w: 3.0, h: 4.0 }];
    assert!(near(total_area(&v), 14.0), "total_area of two rects should be 14.0, got {}", total_area(&v));
    let empty: Vec<Circle> = Vec::new();
    assert!(near(total_area(&empty), 0.0), "total_area of an empty slice should be 0.0");
}

#[test]
fn describe_largest_picks_the_biggest() {
    let shapes: Vec<Box<dyn Shape>> = vec![Box::new(Square(2.0)), Box::new(Circle { r: 2.0 }), Box::new(Rect { w: 3.0, h: 3.0 })];
    assert_eq!(describe_largest(&shapes), Some("circle (area 12.6)".to_string()), "the circle (area 12.57) is the largest");
}

#[test]
fn describe_largest_when_first_or_last_wins() {
    let first: Vec<Box<dyn Shape>> = vec![Box::new(Rect { w: 10.0, h: 10.0 }), Box::new(Square(1.0))];
    assert_eq!(describe_largest(&first), Some("rect (area 100.0)".to_string()), "the rect should win");
    let last: Vec<Box<dyn Shape>> = vec![Box::new(Square(1.0)), Box::new(Square(5.0))];
    assert_eq!(describe_largest(&last), Some("square (area 25.0)".to_string()), "the big square should win");
}

#[test]
fn describe_largest_of_nothing_is_none() {
    let shapes: Vec<Box<dyn Shape>> = Vec::new();
    assert_eq!(describe_largest(&shapes), None, "an empty slice has no largest shape");
}

#[test]
fn report_joins_in_order() {
    let shapes: Vec<Box<dyn Shape>> = vec![Box::new(Square(2.0)), Box::new(Rect { w: 1.0, h: 2.0 }), Box::new(Circle { r: 1.0 })];
    assert_eq!(report(&shapes), "square (area 4.0); rect (area 2.0); circle (area 3.1)", "report should join summaries with \"; \"");
}

#[test]
fn report_of_one_and_none() {
    let one: Vec<Box<dyn Shape>> = vec![Box::new(Square(1.0))];
    assert_eq!(report(&one), "square (area 1.0)", "one shape has no separator");
    let none: Vec<Box<dyn Shape>> = Vec::new();
    assert_eq!(report(&none), "", "no shapes give an empty string");
}
