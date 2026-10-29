// Try it with Run.
fn main() {
    let global = Env::new();
    global.define("x", Value::Int(1));
    let inner = global.child();
    inner.define("y", Value::Int(2));
    inner.assign("x", Value::Int(10));
    println!("{:?} {:?} {:?}", global.get("x"), global.get("y"), inner.get("y"));
    println!("depth {}", inner.depth());
}
