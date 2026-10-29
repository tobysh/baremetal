#[test]
fn expression_results_match_the_tree_walker() {
    for src in ["0", "-7 / 2", "10 - 4 - 3", "100 % 7", "1+2*3", "(1+2)*(3+4)",
        "!false", "3 <= 3", "7 > 10", "true != false", "(1<2)==(3<4)"] {
        let expr = parse_expression(src).unwrap();
        assert_eq!(eval_via_vm(src), eval(&expr, &Env::new()).map_err(Error::from), "{src}");
    }
}

#[test]
fn checked_errors_are_preserved() {
    for (src, err) in [("1/0", RuntimeError::DivByZero), ("7%0", RuntimeError::DivByZero),
        ("9223372036854775807+1", RuntimeError::Overflow),
        ("(-9223372036854775807-1)/-1", RuntimeError::Overflow),
        ("-(-9223372036854775807-1)", RuntimeError::Overflow),
        ("missing", RuntimeError::Undefined("missing".into()))] {
        assert_eq!(eval_via_vm(src), Err(Error::Runtime(err)), "{src}");
    }
    assert!(matches!(eval_via_vm("1+true"), Err(Error::Runtime(RuntimeError::TypeMismatch(_)))));
}

#[test]
fn globals_stack_and_output_survive_multiple_runs() {
    let mut vm = Vm::new();
    vm.run(&[Op::Const(9), Op::Store("x".into()), Op::Load("x".into()), Op::Const(2), Op::Binary(BinOp::Sub), Op::Assign("x".into())]).unwrap();
    assert_eq!(vm.globals.get("x"), Some(&Value::Int(7)));
    assert!(vm.stack.is_empty());
    vm.run(&[Op::Load("x".into()), Op::Print, Op::Bool(true), Op::Print, Op::Const(44)]).unwrap();
    assert_eq!(vm.out, vec!["7", "true"]);
    assert_eq!(vm.stack, vec![Value::Int(44)]);
    vm.run(&[Op::Pop, Op::Const(20), Op::Store("x".into())]).unwrap();
    assert!(vm.stack.is_empty());
    assert_eq!(vm.globals.get("x"), Some(&Value::Int(20)));
    vm.run(&[]).unwrap();
    assert_eq!(vm.out, vec!["7", "true"]);
}

#[test]
fn jumps_use_absolute_addresses_and_pop_conditions() {
    let mut vm = Vm::new();
    vm.run(&[Op::Bool(false), Op::JumpIfFalse(4), Op::Const(99), Op::Print,
        Op::Const(8), Op::Print, Op::Jump(9), Op::Const(77), Op::Print]).unwrap();
    assert_eq!(vm.out, vec!["8"]);
    assert!(vm.stack.is_empty());
    vm.run(&[Op::Bool(true), Op::JumpIfFalse(4), Op::Const(3), Op::Print]).unwrap();
    assert_eq!(vm.out, vec!["8", "3"]);
    assert!(vm.stack.is_empty());
}

#[test]
fn backward_jumps_form_a_bounded_loop() {
    let mut vm = Vm::new();
    vm.run(&[Op::Const(0), Op::Store("i".into()),
        Op::Load("i".into()), Op::Const(4), Op::Binary(BinOp::Lt), Op::JumpIfFalse(13),
        Op::Load("i".into()), Op::Print,
        Op::Load("i".into()), Op::Const(1), Op::Binary(BinOp::Add), Op::Assign("i".into()),
        Op::Jump(2)]).unwrap();
    assert_eq!(vm.out, vec!["0", "1", "2", "3"]);
    assert!(vm.stack.is_empty());
}

#[test]
fn failures_stop_execution_and_underflow_is_an_error() {
    for op in [Op::Pop, Op::Print, Op::Unary(UnOp::Neg), Op::Binary(BinOp::Add),
        Op::Store("x".into()), Op::Assign("x".into()), Op::JumpIfFalse(0)] {
        assert_eq!(Vm::new().run(&[op]), Err(RuntimeError::TypeMismatch("stack underflow".into())));
    }
    let mut vm = Vm::new();
    assert_eq!(vm.run(&[Op::Const(1), Op::Assign("missing".into()), Op::Const(3), Op::Print]), Err(RuntimeError::Undefined("missing".into())));
    assert!(vm.out.is_empty());
    assert!(vm.stack.is_empty());
    assert_eq!(Vm::new().run(&[Op::Const(1), Op::JumpIfFalse(2)]), Err(RuntimeError::TypeMismatch("condition must be a boolean".into())));
}
