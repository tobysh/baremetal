pub type BinOp = extern "C" fn(i32, i32) -> i32;

extern "C" fn add(a: i32, b: i32) -> i32 {
    0
}

extern "C" fn mul(a: i32, b: i32) -> i32 {
    0
}

extern "C" fn max(a: i32, b: i32) -> i32 {
    0
}

fn lookup(name: &str) -> Option<BinOp> {
    None
}

fn fold_with(op: BinOp, init: i32, xs: &[i32]) -> i32 {
    0
}

fn names() -> Vec<&'static str> {
    Vec::new()
}

// Try it with Run.
fn main() {
    println!("{:?}", names());
    if let Some(op) = lookup("add") {
        println!("{}", fold_with(op, 0, &[1, 2, 3]));
    }
}
