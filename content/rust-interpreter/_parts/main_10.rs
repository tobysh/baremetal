// Try it with Run.
fn main() {
    println!("{}", report("let a = 1;\nlet b = 2 3;\nprint a;"));
    println!("{}", report("print 7 / 0;"));
    println!("{}", report("print 1 + 2;"));
}
