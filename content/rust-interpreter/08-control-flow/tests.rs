fn out(src: &str) -> Vec<String> {
    match run(src) {
        Ok(v) => v,
        Err(e) => panic!("{:?} should run but gave {:?}", src, e),
    }
}

fn lines(v: &[&str]) -> Vec<String> {
    v.iter().map(|s| s.to_string()).collect()
}

fn perr(src: &str) -> (String, usize) {
    match run(src) {
        Err(Error::Parse(p)) => (p.msg, p.pos),
        other => panic!("{:?} should be a parse error, got {:?}", src, other),
    }
}

#[test]
fn blocks_run_their_statements() {
    assert_eq!(out("{ print 1; print 2; }"), lines(&["1", "2"]), "a block in order");
    assert_eq!(out("{ }"), lines(&[]), "an empty block");
    assert_eq!(out("{ { print 7; } }"), lines(&["7"]), "nested blocks");
}

#[test]
fn block_scope() {
    assert_eq!(
        run("{ let hidden = 1; } print hidden;"),
        Err(Error::Runtime(RuntimeError::Undefined("hidden".to_string()))),
        "a variable declared in a block is gone after it"
    );
    assert_eq!(out("let x = 1; { let x = 2; print x; } print x;"), lines(&["2", "1"]), "shadowing inside a block");
    assert_eq!(out("let x = 1; { x = 5; } print x;"), lines(&["5"]), "assignment reaches the outer variable");
    assert_eq!(out("let x = 1; { let y = x + 1; { let z = y + 1; print z; } } print x;"), lines(&["3", "1"]), "inner blocks see outer names");
}

#[test]
fn if_without_else() {
    assert_eq!(out("if true { print 1; }"), lines(&["1"]), "true runs the body");
    assert_eq!(out("if false { print 1; } print 2;"), lines(&["2"]), "false skips it");
    assert_eq!(out("let n = 5; if n > 3 { print n; }"), lines(&["5"]), "a comparison condition");
    assert_eq!(out("if 1 + 1 == 2 { print 100; }"), lines(&["100"]), "an arithmetic condition");
}

#[test]
fn if_else() {
    assert_eq!(out("if 1 > 2 { print 1; } else { print 2; }"), lines(&["2"]), "else branch");
    assert_eq!(out("if 2 > 1 { print 1; } else { print 2; }"), lines(&["1"]), "then branch");
    assert_eq!(out("if false { print 1; } else { }"), lines(&[]), "empty else");
}

#[test]
fn else_if_chains() {
    let src = |n: i64| {
        format!(
            "let n = {}; if n < 0 {{ print 0 - 1; }} else if n == 0 {{ print 0; }} else if n < 10 {{ print 1; }} else {{ print 2; }}",
            n
        )
    };
    assert_eq!(out(&src(-5)), lines(&["-1"]), "negative");
    assert_eq!(out(&src(0)), lines(&["0"]), "zero");
    assert_eq!(out(&src(7)), lines(&["1"]), "small");
    assert_eq!(out(&src(70)), lines(&["2"]), "large");
}

#[test]
fn else_if_without_final_else() {
    assert_eq!(out("if false { print 1; } else if false { print 2; }"), lines(&[]), "nothing runs");
    assert_eq!(out("if false { print 1; } else if true { print 2; }"), lines(&["2"]), "second branch");
}

#[test]
fn if_branches_have_their_own_scope() {
    assert_eq!(
        run("if true { let inside = 1; } print inside;"),
        Err(Error::Runtime(RuntimeError::Undefined("inside".to_string()))),
        "then-branch scope"
    );
    assert_eq!(
        run("if false { } else { let e = 1; } print e;"),
        Err(Error::Runtime(RuntimeError::Undefined("e".to_string()))),
        "else-branch scope"
    );
}

#[test]
fn conditions_must_be_boolean() {
    assert!(matches!(run("if 1 { print 1; }"), Err(Error::Runtime(RuntimeError::TypeMismatch(_)))), "if on an integer");
    assert!(matches!(run("while 0 { }"), Err(Error::Runtime(RuntimeError::TypeMismatch(_)))), "while on an integer");
    assert_eq!(run("if 1 / 0 == 1 { }"), Err(Error::Runtime(RuntimeError::DivByZero)), "errors inside the condition surface");
}

#[test]
fn while_loops() {
    assert_eq!(out("let i = 0; while i < 3 { print i; i = i + 1; }"), lines(&["0", "1", "2"]), "counting");
    assert_eq!(out("while false { print 1; } print 2;"), lines(&["2"]), "never runs");
    assert_eq!(
        out("let i = 1; let total = 0; while i <= 100 { total = total + i; i = i + 1; } print total;"),
        lines(&["5050"]),
        "sum to 100"
    );
}

#[test]
fn while_body_gets_a_fresh_scope_each_pass() {
    assert_eq!(
        out("let i = 0; while i < 3 { let sq = i * i; print sq; i = i + 1; }"),
        lines(&["0", "1", "4"]),
        "a let inside the body is fresh each time round"
    );
    assert_eq!(
        run("let i = 0; while i < 2 { let t = i; i = i + 1; } print t;"),
        Err(Error::Runtime(RuntimeError::Undefined("t".to_string()))),
        "and gone after the loop"
    );
}

#[test]
fn nested_loops() {
    let src = "let i = 1;
while i <= 3 {
    let j = 1;
    let row = 0;
    while j <= 3 { row = row + i * j; j = j + 1; }
    print row;
    i = i + 1;
}";
    assert_eq!(out(src), lines(&["6", "12", "18"]), "row sums of a multiplication table");
}

#[test]
fn algorithms() {
    let fib = "let a = 0; let b = 1; let n = 0;
while n < 10 { let next = a + b; a = b; b = next; n = n + 1; }
print a;";
    assert_eq!(out(fib), lines(&["55"]), "tenth Fibonacci number");
    let gcd = "let a = 48; let b = 36;
while b != 0 { let t = b; b = a % b; a = t; }
print a;";
    assert_eq!(out(gcd), lines(&["12"]), "Euclid");
    let collatz = "let n = 27; let steps = 0;
while n != 1 { if n % 2 == 0 { n = n / 2; } else { n = 3 * n + 1; } steps = steps + 1; }
print steps;";
    assert_eq!(out(collatz), lines(&["111"]), "Collatz steps for 27");
    let primes = "let n = 2; let count = 0;
while n < 50 {
    let d = 2; let prime = true;
    while d * d <= n { if n % d == 0 { prime = false; } d = d + 1; }
    if prime { count = count + 1; }
    n = n + 1;
}
print count;";
    assert_eq!(out(primes), lines(&["15"]), "primes below 50");
}

#[test]
fn parse_shapes() {
    assert_eq!(
        parse_program("if a { b; } else { c; }"),
        Ok(vec![Stmt::If(
            Expr::Var("a".to_string()),
            vec![Stmt::ExprStmt(Expr::Var("b".to_string()))],
            Some(vec![Stmt::ExprStmt(Expr::Var("c".to_string()))])
        )]),
        "if/else"
    );
    assert_eq!(
        parse_program("if a { } else if b { }"),
        Ok(vec![Stmt::If(
            Expr::Var("a".to_string()),
            vec![],
            Some(vec![Stmt::If(Expr::Var("b".to_string()), vec![], None)])
        )]),
        "else if nests an if inside the else branch"
    );
    assert_eq!(
        parse_program("while x { { } }"),
        Ok(vec![Stmt::While(Expr::Var("x".to_string()), vec![Stmt::Block(vec![])])]),
        "while containing a block"
    );
}

#[test]
fn parse_errors() {
    assert_eq!(perr("{ print 1;"), ("expected RBrace, found end of input".to_string(), 10), "unclosed block");
    assert_eq!(perr("if true print 1;"), ("expected LBrace, found Print".to_string(), 8), "if needs braces");
    assert_eq!(perr("while { }"), ("unexpected LBrace".to_string(), 6), "while needs a condition");
    assert_eq!(perr("if true { } else print 1;"), ("expected LBrace, found Print".to_string(), 17), "else needs a block or an if");
    assert_eq!(perr("}"), ("unexpected RBrace".to_string(), 0), "a stray close brace");
}

#[test]
fn helpers_directly() {
    let env = Env::new();
    env.define("n", Value::Int(0));
    let body = parse_program("n = n + 1;").unwrap();
    let cond = parse_expression("n < 4").unwrap();
    assert_eq!(exec_while(&cond, &body, &env), Ok(Flow::Normal), "while finishes normally");
    assert_eq!(env.get("n"), Some(Value::Int(4)), "four passes");
    assert_eq!(exec_block(&body, &env), Ok(Flow::Normal), "block");
    assert_eq!(env.get("n"), Some(Value::Int(5)), "block ran its statement");
}
