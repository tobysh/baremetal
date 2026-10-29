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
    match score {
        90..=100 => 'A',
        80..=89 => 'B',
        70..=79 => 'C',
        60..=69 => 'D',
        0..=59 => 'F',
        _ => '?',
    }
}

fn parse_cmd(line: &str) -> Option<Cmd> {
    let line = line.trim();
    if line == "quit" {
        return Some(Cmd::Quit);
    }
    let Some((word, rest)) = line.split_once(' ') else {
        return None;
    };
    match word {
        "say" if !rest.trim().is_empty() => Some(Cmd::Say(rest.to_string())),
        "move" => {
            let parts: Vec<&str> = rest.split_whitespace().collect();
            match parts.as_slice() {
                [a, b] => Some(Cmd::Move { dx: a.parse().ok()?, dy: b.parse().ok()? }),
                _ => None,
            }
        }
        _ => None,
    }
}

fn classify(p: (i32, i32)) -> &'static str {
    match p {
        (0, 0) => "origin",
        (_, 0) => "x-axis",
        (0, _) => "y-axis",
        (x, y) if x == y || x == -y => "diagonal",
        _ => "plane",
    }
}

fn describe(s: &Shape) -> String {
    match s {
        Shape::Rect { w, h } if w == h => format!("square {}", w),
        Shape::Rect { w, h } => format!("rect {}x{}", w, h),
        Shape::Circle { r } => format!("circle r={}", r),
        Shape::Triangle(a, b, c) if a == b && b == c => "equilateral".to_string(),
        Shape::Triangle(a, b, c) if a == b || b == c || a == c => "isosceles".to_string(),
        Shape::Triangle(..) => "scalene".to_string(),
    }
}

fn bucket(n: i64) -> String {
    match n {
        n if n < 0 => "negative".to_string(),
        0 => "zero".to_string(),
        d @ 1..=9 => format!("digit {}", d),
        10..=99 => "two digits".to_string(),
        _ => "large".to_string(),
    }
}

fn main() {
    println!("{}", grade(85));
}
