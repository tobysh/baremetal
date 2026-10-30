pub type BinOp = extern "C" fn(i32, i32) -> i32;

extern "C" fn add(a: i32, b: i32) -> i32 {
    a.wrapping_add(b)
}

extern "C" fn mul(a: i32, b: i32) -> i32 {
    a.wrapping_mul(b)
}

extern "C" fn max(a: i32, b: i32) -> i32 {
    if a > b { a } else { b }
}

const TABLE: [(&str, BinOp); 3] = [("add", add), ("max", max), ("mul", mul)];

fn lookup(name: &str) -> Option<BinOp> {
    TABLE.iter().find(|(n, _)| *n == name).map(|(_, f)| *f)
}

fn fold_with(op: BinOp, init: i32, xs: &[i32]) -> i32 {
    xs.iter().fold(init, |acc, &x| op(acc, x))
}

fn names() -> Vec<&'static str> {
    TABLE.iter().map(|(n, _)| *n).collect()
}

fn main() {
    println!("{:?}", names());
    if let Some(op) = lookup("add") {
        println!("{}", fold_with(op, 0, &[1, 2, 3]));
    }
}
