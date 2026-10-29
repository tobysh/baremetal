// Try it with Run.
fn main() {
    for src in ["1 + 2 * 3", "-(4 - 1) * 2", "f(1, g(2)) < 3", "(1 +"] {
        match parse_expression(src) {
            Ok(e) => println!("{} => {}", src, show(&e)),
            Err(e) => println!("{} => error at {}: {}", src, e.pos, e.msg),
        }
    }
}
