fn ev(src: &str) -> Result<Value, RuntimeError> {
    eval(&parse_expression(src).unwrap())
}

fn int(n: i64) -> Result<Value, RuntimeError> {
    Ok(Value::Int(n))
}

fn boolean(b: bool) -> Result<Value, RuntimeError> {
    Ok(Value::Bool(b))
}

fn is_type_error(r: Result<Value, RuntimeError>) -> bool {
    matches!(r, Err(RuntimeError::TypeMismatch(_)))
}

#[test]
fn literals() {
    assert_eq!(ev("5"), int(5), "number");
    assert_eq!(ev("true"), boolean(true), "true");
    assert_eq!(ev("false"), boolean(false), "false");
}

#[test]
fn arithmetic() {
    assert_eq!(ev("1 + 2 * 3"), int(7), "precedence");
    assert_eq!(ev("(1 + 2) * 3"), int(9), "parens");
    assert_eq!(ev("10 - 4 - 3"), int(3), "left associativity");
    assert_eq!(ev("100 / 7"), int(14), "integer division");
    assert_eq!(ev("100 % 7"), int(2), "remainder");
    assert_eq!(ev("2 * 3 * 4 * 5"), int(120), "product");
    assert_eq!(ev("-7 + 10"), int(3), "negation");
    assert_eq!(ev("--7"), int(7), "double negation");
}

#[test]
fn division_truncates_toward_zero() {
    assert_eq!(ev("-7 / 2"), int(-3), "-7 / 2");
    assert_eq!(ev("7 / -2"), int(-3), "7 / -2");
    assert_eq!(ev("-7 % 3"), int(-1), "remainder takes the sign of the dividend");
    assert_eq!(ev("0 / 5"), int(0), "zero divided");
}

#[test]
fn comparisons() {
    assert_eq!(ev("1 < 2"), boolean(true), "<");
    assert_eq!(ev("2 < 2"), boolean(false), "< equal");
    assert_eq!(ev("2 <= 2"), boolean(true), "<=");
    assert_eq!(ev("3 > 4"), boolean(false), ">");
    assert_eq!(ev("4 >= 4"), boolean(true), ">=");
    assert_eq!(ev("1 + 1 == 2"), boolean(true), "== after arithmetic");
    assert_eq!(ev("1 != 1"), boolean(false), "!=");
    assert_eq!(ev("-1 < 0"), boolean(true), "negatives");
}

#[test]
fn boolean_logic() {
    assert_eq!(ev("!true"), boolean(false), "!true");
    assert_eq!(ev("!false"), boolean(true), "!false");
    assert_eq!(ev("!!true"), boolean(true), "double not");
    assert_eq!(ev("true == true"), boolean(true), "bool ==");
    assert_eq!(ev("true != false"), boolean(true), "bool !=");
    assert_eq!(ev("false == false"), boolean(true), "false == false");
    assert_eq!(ev("(1 < 2) == (3 < 4)"), boolean(true), "comparing comparisons");
    assert_eq!(ev("!(1 == 2)"), boolean(true), "not of a comparison");
}

#[test]
fn division_by_zero() {
    assert_eq!(ev("1 / 0"), Err(RuntimeError::DivByZero), "1 / 0");
    assert_eq!(ev("5 % 0"), Err(RuntimeError::DivByZero), "5 % 0");
    assert_eq!(ev("1 / (2 - 2)"), Err(RuntimeError::DivByZero), "zero computed");
    assert_eq!(ev("0 / 0"), Err(RuntimeError::DivByZero), "0 / 0");
}

#[test]
fn overflow() {
    assert_eq!(ev("9223372036854775807 + 1"), Err(RuntimeError::Overflow), "add");
    assert_eq!(ev("0 - 9223372036854775807 - 2"), Err(RuntimeError::Overflow), "sub");
    assert_eq!(ev("9223372036854775807 * 2"), Err(RuntimeError::Overflow), "mul");
    assert_eq!(ev("9223372036854775807 + 0"), int(i64::MAX), "no overflow at the limit");
    assert_eq!(ev("-9223372036854775807 - 1"), int(i64::MIN), "reaching i64::MIN is fine");
    assert_eq!(ev("(-9223372036854775807 - 1) / -1"), Err(RuntimeError::Overflow), "MIN / -1");
    assert_eq!(ev("(-9223372036854775807 - 1) % -1"), Err(RuntimeError::Overflow), "MIN % -1");
    assert_eq!(ev("-(-9223372036854775807 - 1)"), Err(RuntimeError::Overflow), "negating MIN");
}

#[test]
fn type_errors() {
    assert!(is_type_error(ev("1 + true")), "int + bool");
    assert!(is_type_error(ev("true + true")), "bool + bool");
    assert!(is_type_error(ev("true < false")), "ordering bools");
    assert!(is_type_error(ev("1 == true")), "comparing different kinds");
    assert!(is_type_error(ev("-true")), "negating a bool");
    assert!(is_type_error(ev("!5")), "not of an int");
    assert!(is_type_error(ev("true * 2")), "bool * int");
    assert!(is_type_error(ev("1 / false")), "int / bool");
}

#[test]
fn errors_win_left_to_right() {
    assert_eq!(ev("(1 / 0) + (9223372036854775807 + 1)"), Err(RuntimeError::DivByZero), "left error first");
    assert_eq!(ev("(9223372036854775807 + 1) + (1 / 0)"), Err(RuntimeError::Overflow), "left error first, other way");
    assert_eq!(ev("1 / 0 + x"), Err(RuntimeError::DivByZero), "an error on the left hides the undefined variable on the right");
}

#[test]
fn variables_do_not_exist_yet() {
    assert_eq!(ev("x"), Err(RuntimeError::Undefined("x".to_string())), "any name is undefined");
    assert_eq!(ev("1 + total"), Err(RuntimeError::Undefined("total".to_string())), "inside an expression");
}

#[test]
fn calls_are_not_supported_yet() {
    assert_eq!(ev("f(1)"), Err(RuntimeError::NotCallable), "calling an undefined name is NotCallable for now");
    assert_eq!(ev("1(2)"), Err(RuntimeError::NotCallable), "calling a number");
}

#[test]
fn apply_helpers_directly() {
    assert_eq!(apply_unary(UnOp::Neg, &Value::Int(5)), int(-5), "apply_unary neg");
    assert_eq!(apply_unary(UnOp::Not, &Value::Bool(false)), boolean(true), "apply_unary not");
    assert_eq!(apply_binary(BinOp::Add, &Value::Int(2), &Value::Int(3)), int(5), "apply_binary add");
    assert_eq!(apply_binary(BinOp::Ge, &Value::Int(2), &Value::Int(3)), boolean(false), "apply_binary >=");
    assert_eq!(apply_binary(BinOp::Ne, &Value::Bool(true), &Value::Bool(true)), boolean(false), "apply_binary bool !=");
    assert!(is_type_error(apply_binary(BinOp::Add, &Value::Int(1), &Value::Bool(true))), "mixed kinds");
}

#[test]
fn display_of_values() {
    assert_eq!(Value::Int(-4).to_string(), "-4", "int");
    assert_eq!(Value::Bool(true).to_string(), "true", "bool");
    assert_eq!(RuntimeError::DivByZero.to_string(), "division by zero", "error text");
}
