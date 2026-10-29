// Try it with Run.
fn main() {
    println!("{}", transcript(&["let x = 5;", "x * 2", "fn sq(n) { return n * n; }", "sq(x)", "y"]));
}
