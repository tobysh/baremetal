fn out(src: &str) -> Vec<String> {
    match run(src) {
        Ok(v) => v,
        Err(e) => panic!("{:?} should run but gave {:?}", src, e),
    }
}

fn lines(v: &[&str]) -> Vec<String> {
    v.iter().map(|s| s.to_string()).collect()
}

#[test]
fn print_statements() {
    assert_eq!(out("print 1;"), lines(&["1"]), "print a number");
    assert_eq!(out("print 1 + 2 * 3; print 10 / 3;"), lines(&["7", "3"]), "two prints in order");
    assert_eq!(out("print true; print 2 > 3;"), lines(&["true", "false"]), "booleans");
    assert_eq!(out(""), lines(&[]), "an empty program prints nothing");
}

#[test]
fn let_and_use() {
    assert_eq!(out("let a = 6; let b = a * 7; print b;"), lines(&["42"]), "variables in later statements");
    assert_eq!(out("let x = 1; let y = x + 1; let z = y + 1; print x; print y; print z;"), lines(&["1", "2", "3"]), "a chain");
    assert_eq!(out("let t = 1 < 2; print t;"), lines(&["true"]), "storing a boolean");
}

#[test]
fn let_redefines() {
    assert_eq!(out("let x = 1; let x = x + 10; print x;"), lines(&["11"]), "let can reuse a name; the right side sees the old value");
}

#[test]
fn assignment() {
    assert_eq!(out("let n = 1; n = n + 1; n = n * 10; print n;"), lines(&["20"]), "assignment updates");
    assert_eq!(out("let a = 1; let b = 2; a = b; b = 9; print a; print b;"), lines(&["2", "9"]), "values are copied");
}

#[test]
fn assigning_unknown_variable_fails() {
    assert_eq!(run("x = 1;"), Err(Error::Runtime(RuntimeError::Undefined("x".to_string()))), "assignment needs let first");
}

#[test]
fn expression_statements() {
    assert_eq!(out("1 + 2; print 5;"), lines(&["5"]), "the value is discarded");
    assert_eq!(out("let a = 1; a; a + 1;"), lines(&[]), "nothing printed");
    assert_eq!(run("1 / 0;"), Err(Error::Runtime(RuntimeError::DivByZero)), "but errors still happen");
    assert_eq!(run("nope;"), Err(Error::Runtime(RuntimeError::Undefined("nope".to_string()))), "reading an undefined variable");
}

#[test]
fn runtime_errors_stop_the_program() {
    assert_eq!(run("print 1; print 1 / 0; print 2;"), Err(Error::Runtime(RuntimeError::DivByZero)), "the first error ends the run");
    assert!(matches!(run("let x = 1 + true;"), Err(Error::Runtime(RuntimeError::TypeMismatch(_)))), "type error inside let");
}

#[test]
fn assignment_statement_vs_comparison_expression() {
    assert_eq!(out("let a = 1; print a == 1;"), lines(&["true"]), "== is a comparison");
    assert_eq!(out("let a = 1; a == 2; print a;"), lines(&["1"]), "a == 2 as a statement changes nothing");
}

#[test]
fn parse_errors_point_at_the_problem() {
    let e = |src: &str| match run(src) {
        Err(Error::Parse(p)) => (p.msg, p.pos),
        other => panic!("{:?} should be a parse error, got {:?}", src, other),
    };
    assert_eq!(e("print 1"), ("expected Semi, found end of input".to_string(), 7), "missing semicolon at the end");
    assert_eq!(e("let x = 1 print x;"), ("expected Semi, found Print".to_string(), 10), "missing semicolon in the middle");
    assert_eq!(e("let = 5;"), ("expected identifier, found Eq".to_string(), 4), "missing name");
    assert_eq!(e("let 5 = 5;"), ("expected identifier, found Num(5)".to_string(), 4), "bad name");
    assert_eq!(e("let x 5;"), ("expected Eq, found Num(5)".to_string(), 6), "missing =");
    assert_eq!(e("print ;"), ("unexpected Semi".to_string(), 6), "print needs an expression");
    assert_eq!(e("x = ;"), ("unexpected Semi".to_string(), 4), "assignment needs a value");
    assert_eq!(e("1 + 2 3;"), ("expected Semi, found Num(3)".to_string(), 6), "junk after an expression");
    assert_eq!(e("let x = 1; $"), ("unexpected character '$'".to_string(), 11), "lexer errors in the middle");
}

#[test]
fn parse_program_shapes() {
    assert_eq!(
        parse_program("let a = 1; a = 2; print a; a;"),
        Ok(vec![
            Stmt::Let("a".to_string(), Expr::Num(1)),
            Stmt::Assign("a".to_string(), Expr::Num(2)),
            Stmt::Print(Expr::Var("a".to_string())),
            Stmt::ExprStmt(Expr::Var("a".to_string())),
        ]),
        "one of each statement"
    );
    assert_eq!(parse_program(""), Ok(vec![]), "empty program");
    assert_eq!(parse_program("# only a comment\n"), Ok(vec![]), "comments only");
}

#[test]
fn comments_and_layout() {
    assert_eq!(out("let a = 2; # two\nprint a * a; # four"), lines(&["4"]), "comments to end of line");
    assert_eq!(out("let\n  a\n  =\n  3\n;\nprint a;"), lines(&["3"]), "statements may span lines");
}

#[test]
fn run_in_keeps_the_environment() {
    let env = Env::new();
    assert_eq!(run_in("let x = 10;", &env), Ok(vec![]), "first run defines x");
    assert_eq!(run_in("x = x + 5; print x;", &env), Ok(vec!["15".to_string()]), "second run sees it");
    assert_eq!(env.get("x"), Some(Value::Int(15)), "and the environment holds it");
}

#[test]
fn exec_stmt_directly() {
    let env = Env::new();
    assert_eq!(exec_stmt(&Stmt::Let("k".to_string(), Expr::Num(3)), &env), Ok(Flow::Normal), "let returns Normal");
    assert_eq!(env.get("k"), Some(Value::Int(3)), "and defines");
    assert_eq!(exec_stmt(&Stmt::Assign("zz".to_string(), Expr::Num(1)), &env), Err(RuntimeError::Undefined("zz".to_string())), "assign to nothing");
    assert_eq!(exec_stmt(&Stmt::Print(Expr::Num(8)), &env), Ok(Flow::Normal), "print returns Normal");
    assert_eq!(env.take_output(), vec!["8".to_string()], "and emits");
}

#[test]
fn a_longer_program() {
    let src = "let width = 6; let height = 4;
let area = width * height;
print area;
let perimeter = 2 * (width + height);
print perimeter;
print area > perimeter;
width = width + 1;
print width * height;";
    assert_eq!(out(src), lines(&["24", "20", "true", "28"]), "rectangle");
}
