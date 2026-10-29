// Try it with Run.
fn main() {
    let src = "let a = 6;\nlet b = a * 7;\nprint b;\na = a + 1;\nprint a < b;";
    println!("{:?}", run(src));
    println!("{:?}", run("print missing;"));
}
