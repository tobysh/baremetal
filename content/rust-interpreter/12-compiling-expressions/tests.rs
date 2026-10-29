fn code(src: &str) -> Vec<Op> { compile_expr(&parse_expression(src).unwrap()).unwrap() }

#[test]
fn literals_and_loads() {
    for n in [0, 1, 712, i64::MAX] { assert_eq!(code(&n.to_string()), vec![Op::Const(n)]); }
    assert_eq!(code("true"), vec![Op::Bool(true)]);
    assert_eq!(code("false"), vec![Op::Bool(false)]);
    assert_eq!(code("count"), vec![Op::Load("count".into())]);
}

#[test]
fn operators_are_postfix_in_source_order() {
    assert_eq!(code("1 + 2 * 3"), vec![Op::Const(1), Op::Const(2), Op::Const(3), Op::Binary(BinOp::Mul), Op::Binary(BinOp::Add)]);
    assert_eq!(code("10 - 4 - 3"), vec![Op::Const(10), Op::Const(4), Op::Binary(BinOp::Sub), Op::Const(3), Op::Binary(BinOp::Sub)]);
    assert_eq!(code("-x + !false"), vec![Op::Load("x".into()), Op::Unary(UnOp::Neg), Op::Bool(false), Op::Unary(UnOp::Not), Op::Binary(BinOp::Add)]);
    assert_eq!(code("(a + b) <= c"), vec![Op::Load("a".into()), Op::Load("b".into()), Op::Binary(BinOp::Add), Op::Load("c".into()), Op::Binary(BinOp::Le)]);
}

#[test]
fn every_binary_operator_keeps_its_identity() {
    for (text, op) in [("+", BinOp::Add), ("-", BinOp::Sub), ("*", BinOp::Mul),
        ("/", BinOp::Div), ("%", BinOp::Rem), ("==", BinOp::Eq), ("!=", BinOp::Ne),
        ("<", BinOp::Lt), ("<=", BinOp::Le), (">", BinOp::Gt), (">=", BinOp::Ge)] {
        assert_eq!(code(&format!("11 {text} 3")), vec![Op::Const(11), Op::Const(3), Op::Binary(op)]);
    }
}

#[test]
fn nested_calls_are_explicitly_unsupported() {
    for src in ["f()", "f(1, 2)", "1 + f(3)", "-f()", "f()(1)"] {
        assert_eq!(compile_expr(&parse_expression(src).unwrap()), Err(CompileError::Unsupported("calls".into())), "{src}");
    }
}

#[test]
fn stack_depth_follows_shape() {
    assert_eq!(max_stack(&[]), 0);
    for (src, want) in [("1", 1), ("--1", 1), ("1+2+3+4", 2),
        ("1+(2+(3+4))", 4), ("(1+2)*(3+4)", 3), ("!(x == 1)", 2),
        ("a+(b*c)+(d*(e+f))", 4)] {
        assert_eq!(max_stack(&code(src)), want, "{src}");
    }
}
