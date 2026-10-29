// Try it with Run.
fn main() {
    let src = "fn make_counter() {
    let n = 0;
    fn next() { n = n + 1; return n; }
    return next;
}
let c = make_counter();
c(); c();
print c();";
    println!("{:?}", run(src));
}
