// Try it with Run.
fn main() {
    let src = "let i = 1; let total = 0;
while i <= 5 { total = total + i; i = i + 1; }
if total > 10 { print total; } else { print 0; }";
    println!("{:?}", run(src));
}
