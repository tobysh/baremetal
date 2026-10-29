`grade`: `match score { 90..=100 => 'A', 80..=89 => 'B', 70..=79 => 'C', 60..=69 => 'D', 0..=59 => 'F', _ => '?' }`. `bucket`: `n if n < 0 => ..., 0 => ..., d @ 1..=9 => format!("digit {d}"), 10..=99 => ..., _ => ...`.
===
`parse_cmd`: `let line = line.trim(); if line == "quit" { return Some(Cmd::Quit); } let Some((word, rest)) = line.split_once(' ') else { return None; };` then `match word { "say" if !rest.trim().is_empty() => ..., "move" => ..., _ => None }`. For move: `let parts: Vec<&str> = rest.split_whitespace().collect(); match parts.as_slice() { [a, b] => ..., _ => None }`. Parse with `a.parse().ok()?`.
===
`classify`: `match p { (0, 0) => "origin", (_, 0) => "x-axis", (0, _) => "y-axis", (x, y) if x == y || x == -y => "diagonal", _ => "plane" }`. `describe` for Triangle: `Shape::Triangle(a, b, c) if a == b && b == c => ..., Shape::Triangle(a, b, c) if a == b || b == c || a == c => ..., Shape::Triangle(..) => ...`. For Rect use a guard `if w == h`.
