// Try it with Run.
fn main() {
    let src = "let i = 0; while i < 3 { print i * i; i = i + 1; }";
    println!("{:?}", compile_stmts(&parse_program(src).unwrap()));
    println!("{:?}", run_vm(src));
}
