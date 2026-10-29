// Try it with Run.
fn main() {
    println!("{:?}", lex("let x = 4 + 20;"));
    println!("{:?}", lex("a <= b != c"));
    println!("{:?}", lex("1 $ 2"));
}
