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
    match e {
        Expr::Num(n) => Ok(*n),
        Expr::Neg(a) => eval(a)?.checked_neg().ok_or(EvalError::Overflow),
        Expr::Add(a, b) => {
            let (x, y) = (eval(a)?, eval(b)?);
            x.checked_add(y).ok_or(EvalError::Overflow)
        }
        Expr::Sub(a, b) => {
            let (x, y) = (eval(a)?, eval(b)?);
            x.checked_sub(y).ok_or(EvalError::Overflow)
        }
        Expr::Mul(a, b) => {
            let (x, y) = (eval(a)?, eval(b)?);
            x.checked_mul(y).ok_or(EvalError::Overflow)
        }
        Expr::Div(a, b) => {
            let (x, y) = (eval(a)?, eval(b)?);
            if y == 0 {
                return Err(EvalError::DivByZero);
            }
            x.checked_div(y).ok_or(EvalError::Overflow)
        }
    }
}

fn simplify(e: Expr) -> Expr {
    match e {
        Expr::Num(n) => Expr::Num(n),
        Expr::Neg(inner) => match simplify(*inner) {
            Expr::Neg(x) => *x,
            Expr::Num(n) => Expr::Num(n.wrapping_neg()),
            other => neg(other),
        },
        Expr::Add(a, b) => match (simplify(*a), simplify(*b)) {
            (Expr::Num(0), x) | (x, Expr::Num(0)) => x,
            (a, b) => add(a, b),
        },
        Expr::Sub(a, b) => match (simplify(*a), simplify(*b)) {
            (x, Expr::Num(0)) => x,
            (a, b) => sub(a, b),
        },
        Expr::Mul(a, b) => match (simplify(*a), simplify(*b)) {
            (Expr::Num(0), _) | (_, Expr::Num(0)) => Expr::Num(0),
            (Expr::Num(1), x) | (x, Expr::Num(1)) => x,
            (a, b) => mul(a, b),
        },
        Expr::Div(a, b) => match (simplify(*a), simplify(*b)) {
            (x, Expr::Num(1)) => x,
            (a, b) => div(a, b),
        },
    }
}

fn describe(items: &[i32]) -> String {
    match items {
        [] => "empty".to_string(),
        [x] => format!("one: {}", x),
        [a, b] => format!("two: {},{}", a, b),
        [first, .., last] if first == last => format!("bookends: {}", first),
        [first, rest @ ..] => format!("{} then {} more", first, rest.len()),
    }
}

// Try it with Run.
fn main() {
    let e = add(num(2), mul(num(3), num(4)));
    println!("{:?}", eval(&e));
    println!("{:?}", simplify(mul(num(1), add(num(0), num(9)))));
    println!("{}", describe(&[1, 2, 3, 1]));
}
