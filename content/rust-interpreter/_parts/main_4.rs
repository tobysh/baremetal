// Try it with Run.
fn main() {
    for src in ["1 + 2 * 3", "10 / (5 - 5)", "1 < 2 == true", "-true"] {
        let result = parse_expression(src).map(|e| eval(&e));
        println!("{} => {:?}", src, result);
    }
}
