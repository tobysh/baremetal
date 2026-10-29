// Try it with Run.
fn main() {
    let mut s = Stream::from_source("let x = 1;").unwrap();
    println!("{:?} / {:?}", s.peek(), s.peek_at(1));
    println!("{:?}", s.next());
    println!("{:?} then pos {}", s.expect(&Token::Semi), s.pos());
}
