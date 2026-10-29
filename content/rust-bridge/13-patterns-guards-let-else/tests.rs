#[test]
fn grades() {
    let cases = [(100, 'A'), (90, 'A'), (89, 'B'), (80, 'B'), (79, 'C'), (70, 'C'), (69, 'D'), (60, 'D'), (59, 'F'), (0, 'F'), (101, '?'), (5000, '?')];
    for (score, want) in cases {
        assert_eq!(grade(score), want, "score {}", score);
    }
}

#[test]
fn commands() {
    assert_eq!(parse_cmd("quit"), Some(Cmd::Quit), "quit");
    assert_eq!(parse_cmd("  quit  "), Some(Cmd::Quit), "quit with spaces");
    assert_eq!(parse_cmd("say hello big world"), Some(Cmd::Say("hello big world".to_string())), "say keeps all the text");
    assert_eq!(parse_cmd("move 3 -4"), Some(Cmd::Move { dx: 3, dy: -4 }), "move");
    assert_eq!(parse_cmd("move  0   0"), Some(Cmd::Move { dx: 0, dy: 0 }), "extra spaces");
}

#[test]
fn bad_commands() {
    for bad in ["", "say", "say ", "move 1", "move 1 2 3", "move a b", "move 1 x", "jump 1 2", "quit now", "Quit"] {
        assert_eq!(parse_cmd(bad), None, "{:?} is not a command", bad);
    }
}

#[test]
fn points() {
    assert_eq!(classify((0, 0)), "origin", "origin");
    assert_eq!(classify((5, 0)), "x-axis", "x-axis");
    assert_eq!(classify((-5, 0)), "x-axis", "negative x-axis");
    assert_eq!(classify((0, 7)), "y-axis", "y-axis");
    assert_eq!(classify((3, 3)), "diagonal", "x == y");
    assert_eq!(classify((2, -2)), "diagonal", "x == -y");
    assert_eq!(classify((2, 5)), "plane", "elsewhere");
}

#[test]
fn shapes() {
    assert_eq!(describe(&Shape::Rect { w: 5, h: 5 }), "square 5", "square");
    assert_eq!(describe(&Shape::Rect { w: 3, h: 4 }), "rect 3x4", "rectangle");
    assert_eq!(describe(&Shape::Circle { r: 2 }), "circle r=2", "circle");
    assert_eq!(describe(&Shape::Triangle(3, 3, 3)), "equilateral", "equilateral");
    assert_eq!(describe(&Shape::Triangle(3, 3, 4)), "isosceles", "isosceles");
    assert_eq!(describe(&Shape::Triangle(4, 3, 3)), "isosceles", "isosceles, other order");
    assert_eq!(describe(&Shape::Triangle(3, 4, 3)), "isosceles", "isosceles, outer pair");
    assert_eq!(describe(&Shape::Triangle(3, 4, 5)), "scalene", "scalene");
}

#[test]
fn buckets() {
    assert_eq!(bucket(-5), "negative", "negative");
    assert_eq!(bucket(i64::MIN), "negative", "very negative");
    assert_eq!(bucket(0), "zero", "zero");
    assert_eq!(bucket(1), "digit 1", "1");
    assert_eq!(bucket(9), "digit 9", "9");
    assert_eq!(bucket(10), "two digits", "10");
    assert_eq!(bucket(99), "two digits", "99");
    assert_eq!(bucket(100), "large", "100");
}
