#[derive(Debug, PartialEq)]
enum Cmd {
    Move { dx: i32, dy: i32 },
    Say(String),
    Quit,
}

enum Shape {
    Rect { w: u32, h: u32 },
    Circle { r: u32 },
    Triangle(u32, u32, u32),
}

fn grade(score: u32) -> char {
    '?'
}

fn parse_cmd(line: &str) -> Option<Cmd> {
    None
}

fn classify(p: (i32, i32)) -> &'static str {
    "plane"
}

fn describe(s: &Shape) -> String {
    String::new()
}

fn bucket(n: i64) -> String {
    String::new()
}

// Try it with Run.
fn main() {
    println!("{}", grade(85));
    println!("{:?}", parse_cmd("move 3 -4"));
    println!("{}", classify((2, -2)));
    println!("{}", describe(&Shape::Rect { w: 3, h: 4 }));
    println!("{}", bucket(7));
    let _ = Shape::Circle { r: 1 };
    let _ = Shape::Triangle(1, 1, 1);
}
