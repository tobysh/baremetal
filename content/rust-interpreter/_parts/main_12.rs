// Try it with Run.
fn main() {
    let e = parse_expression("(1 + 2) * x").unwrap();
    let code = compile_expr(&e).unwrap();
    println!("{:?}", code);
    println!("max stack {}", max_stack(&code));
}
