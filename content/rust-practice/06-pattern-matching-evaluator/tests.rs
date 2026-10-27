#[test]
fn eval_numbers_and_arithmetic() {
    assert_eq!(eval(&num(7)), Ok(7), "a number evaluates to itself");
    assert_eq!(eval(&add(num(2), mul(num(3), num(4)))), Ok(14), "2 + 3 * 4");
    assert_eq!(eval(&sub(num(10), num(3))), Ok(7), "10 - 3");
    assert_eq!(eval(&div(num(20), num(6))), Ok(3), "integer division");
    assert_eq!(eval(&neg(sub(num(1), num(5)))), Ok(4), "-(1 - 5)");
}

#[test]
fn eval_division_by_zero() {
    assert_eq!(eval(&div(num(1), num(0))), Err(EvalError::DivByZero), "1 / 0");
    assert_eq!(eval(&div(num(1), sub(num(3), num(3)))), Err(EvalError::DivByZero), "1 / (3 - 3)");
}

#[test]
fn eval_overflow() {
    assert_eq!(eval(&add(num(i64::MAX), num(1))), Err(EvalError::Overflow), "add overflow");
    assert_eq!(eval(&sub(num(i64::MIN), num(1))), Err(EvalError::Overflow), "sub overflow");
    assert_eq!(eval(&mul(num(i64::MAX), num(2))), Err(EvalError::Overflow), "mul overflow");
    assert_eq!(eval(&neg(num(i64::MIN))), Err(EvalError::Overflow), "negating the minimum");
    assert_eq!(eval(&div(num(i64::MIN), num(-1))), Err(EvalError::Overflow), "MIN / -1");
}

#[test]
fn eval_first_error_wins() {
    let e = add(div(num(1), num(0)), add(num(i64::MAX), num(1)));
    assert_eq!(eval(&e), Err(EvalError::DivByZero), "the left side is evaluated first");
}

#[test]
fn simplify_add_and_sub_zero() {
    assert_eq!(simplify(add(num(0), neg(num(0)))), num(0), "0 + (-0) simplifies to 0");
    assert_eq!(simplify(add(mul(num(2), num(3)), num(0))), mul(num(2), num(3)), "x + 0");
    assert_eq!(simplify(add(num(0), sub(num(5), num(2)))), sub(num(5), num(2)), "0 + x");
    assert_eq!(simplify(sub(add(num(1), num(2)), num(0))), add(num(1), num(2)), "x - 0");
    assert_eq!(simplify(sub(num(0), num(5))), sub(num(0), num(5)), "0 - x is not simplified");
}

#[test]
fn simplify_mul_and_div() {
    assert_eq!(simplify(mul(num(1), add(num(2), num(3)))), add(num(2), num(3)), "1 * x");
    assert_eq!(simplify(mul(add(num(2), num(3)), num(1))), add(num(2), num(3)), "x * 1");
    assert_eq!(simplify(mul(add(num(2), num(3)), num(0))), num(0), "x * 0");
    assert_eq!(simplify(mul(num(0), div(num(1), num(0)))), num(0), "0 * x, even if x would fail");
    assert_eq!(simplify(div(sub(num(9), num(1)), num(1))), sub(num(9), num(1)), "x / 1");
    assert_eq!(simplify(div(num(1), num(2))), div(num(1), num(2)), "other divisions stay");
}

#[test]
fn simplify_negation() {
    assert_eq!(simplify(neg(neg(add(num(1), num(2))))), add(num(1), num(2)), "--x");
    assert_eq!(simplify(neg(num(5))), num(-5), "-5");
    assert_eq!(simplify(neg(neg(neg(num(5))))), num(-5), "---5");
    assert_eq!(simplify(neg(add(num(1), num(2)))), neg(add(num(1), num(2))), "other negations stay");
}

#[test]
fn simplify_is_bottom_up() {
    // (x * 1) + (0 * y)  =>  x + 0  =>  x   where x = 1 + 2, y = 3
    let e = add(mul(add(num(1), num(2)), num(1)), mul(num(0), num(3)));
    assert_eq!(simplify(e), add(num(1), num(2)), "children are simplified before their parent");
}

#[test]
fn simplify_leaves_constants_alone() {
    assert_eq!(simplify(add(num(1), num(2))), add(num(1), num(2)), "1 + 2 is not evaluated");
    assert_eq!(simplify(num(9)), num(9), "a plain number");
}

#[test]
fn describe_small_slices() {
    assert_eq!(describe(&[]), "empty", "empty");
    assert_eq!(describe(&[7]), "one: 7", "one");
    assert_eq!(describe(&[3, 4]), "two: 3,4", "two");
    assert_eq!(describe(&[5, 5]), "two: 5,5", "two equal items are still just two");
}

#[test]
fn describe_bookends_and_rest() {
    assert_eq!(describe(&[1, 2, 3, 1]), "bookends: 1", "equal ends");
    assert_eq!(describe(&[9, 9, 9]), "bookends: 9", "three equal");
    assert_eq!(describe(&[4, 5, 6]), "4 then 2 more", "different ends");
    assert_eq!(describe(&[1, 2, 3, 4, 5]), "1 then 4 more", "longer slice");
}
