#[derive(Debug, PartialEq, Clone)]
enum Expr {
    Num(i64),
    Neg(Box<Expr>),
    Add(Box<Expr>, Box<Expr>),
    Sub(Box<Expr>, Box<Expr>),
    Mul(Box<Expr>, Box<Expr>),
    Div(Box<Expr>, Box<Expr>),
}

#[derive(Debug, PartialEq, Clone)]
enum EvalError {
    DivByZero,
    Overflow,
}

fn num(n: i64) -> Expr {
    Expr::Num(n)
}
fn neg(a: Expr) -> Expr {
    Expr::Neg(Box::new(a))
}
fn add(a: Expr, b: Expr) -> Expr {
    Expr::Add(Box::new(a), Box::new(b))
}
fn sub(a: Expr, b: Expr) -> Expr {
    Expr::Sub(Box::new(a), Box::new(b))
}
fn mul(a: Expr, b: Expr) -> Expr {
    Expr::Mul(Box::new(a), Box::new(b))
}
fn div(a: Expr, b: Expr) -> Expr {
    Expr::Div(Box::new(a), Box::new(b))
}

fn eval(e: &Expr) -> Result<i64, EvalError> {
    Err(EvalError::Overflow)
}

fn simplify(e: Expr) -> Expr {
    e
}

fn describe(items: &[i32]) -> String {
    String::new()
}

// Try it with Run.
fn main() {
    let e = add(num(2), mul(num(3), num(4)));
    println!("{:?}", eval(&e));
    println!("{:?}", simplify(mul(num(1), add(num(0), num(9)))));
    println!("{}", describe(&[1, 2, 3, 1]));
}
