fn compiled(src: &str) -> Vec<Op> { compile_stmts(&parse_program(src).unwrap()).unwrap() }

#[test]
fn simple_statements_and_empty_programs() {
    assert_eq!(compiled(""), vec![]);
    assert_eq!(compiled("{}"), vec![]);
    assert_eq!(compiled("let x = 2; x = 7; print x; x + 1;"), vec![
        Op::Const(2), Op::Store("x".into()), Op::Const(7), Op::Assign("x".into()),
        Op::Load("x".into()), Op::Print, Op::Load("x".into()), Op::Const(1), Op::Binary(BinOp::Add), Op::Pop]);
}

#[test]
fn exact_branch_targets() {
    assert_eq!(compiled("if true { print 1; }"), vec![Op::Bool(true), Op::JumpIfFalse(4), Op::Const(1), Op::Print]);
    assert_eq!(compiled("if false { print 1; } else { print 2; }"), vec![
        Op::Bool(false), Op::JumpIfFalse(5), Op::Const(1), Op::Print, Op::Jump(7), Op::Const(2), Op::Print]);
    assert_eq!(compiled("while false { print 1; }"), vec![
        Op::Bool(false), Op::JumpIfFalse(5), Op::Const(1), Op::Print, Op::Jump(0)]);
}

#[test]
fn targets_include_existing_code_and_empty_branches() {
    let mut c = vec![Op::Const(55), Op::Print];
    emit_stmt(&parse_program("while false {} ").unwrap()[0], &mut c).unwrap();
    assert_eq!(c, vec![Op::Const(55), Op::Print, Op::Bool(false), Op::JumpIfFalse(5), Op::Jump(2)]);
    assert_eq!(compiled("if true {} else {}"), vec![Op::Bool(true), Op::JumpIfFalse(3), Op::Jump(3)]);
}

#[test]
fn both_engines_agree_on_the_shared_subset() {
    for limit in [0, 1, 4, 11] {
        let src = format!("let i = 0; let sum = 0; while i < {limit} {{ if i % 2 == 0 {{ sum = sum + i; }} else {{ sum = sum - i; }} i = i + 1; }} print sum; print i;");
        assert_eq!(run_vm(&src), run(&src).map_err(|e| render_error(&src, &e)), "{src}");
        let code = compiled(&src);
        let mut vm = Vm::new();
        vm.run(&code).unwrap();
        assert!(vm.stack.is_empty(), "conditions/statements must leave no stack garbage");
    }
    for src in ["if false { print absent; } else { print 8; }",
        "let i = 0; while i < 3 { let j = 0; while j < 2 { print i * 10 + j; j = j + 1; } i = i + 1; }",
        "{ print 1; { print 2; } } print 3;", "if true { if false { print 9; } else { print 4; } } else { print 8; }"] {
        assert_eq!(run_vm(src), run(src).map_err(|e| render_error(src, &e)), "{src}");
    }
}

#[test]
fn compiled_blocks_have_the_documented_flat_scope() {
    assert_eq!(run_vm("let x = 1; { let x = 2; let y = 7; } print x; print y;"), Ok(vec!["2".into(), "7".into()]));
}

#[test]
fn runtime_diagnostics_and_unsupported_constructs() {
    for src in ["print 1 / 0;", "print absent;", "absent = 3;", "while 1 {}", "if true { print -true; }"] {
        assert_eq!(run_vm(src), run(src).map_err(|e| render_error(src, &e)), "{src}");
    }
    for src in ["fn f() {}", "return;", "if false { return 2; }"] {
        assert_eq!(compile_stmts(&parse_program(src).unwrap()), Err(CompileError::Unsupported("functions".into())), "{src}");
    }
    assert_eq!(compile_stmts(&parse_program("print f();").unwrap()), Err(CompileError::Unsupported("calls".into())));
    assert!(run_vm("print ;").unwrap_err().starts_with("error: unexpected Semi\n"));
}
