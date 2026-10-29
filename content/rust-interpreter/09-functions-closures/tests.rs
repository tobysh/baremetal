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
fn simple_functions() {
    assert_eq!(out("fn add(a, b) { return a + b; } print add(2, 3);"), lines(&["5"]), "two parameters");
    assert_eq!(out("fn five() { return 5; } print five() * 2;"), lines(&["10"]), "no parameters, used in an expression");
    assert_eq!(out("fn sq(n) { return n * n; } print sq(sq(3));"), lines(&["81"]), "nested calls");
    assert_eq!(out("fn id(x) { return x; } print id(true); print id(7);"), lines(&["true", "7"]), "any kind of value");
}

#[test]
fn arguments_are_evaluated_in_the_callers_scope() {
    assert_eq!(out("let k = 10; fn f(a) { return a + 1; } print f(k * 2);"), lines(&["21"]), "argument expressions");
    assert_eq!(out("fn f(x) { return x; } let x = 5; print f(x + 1);"), lines(&["6"]), "same name as the parameter");
}

#[test]
fn return_without_value_and_falling_off_the_end() {
    assert_eq!(out("fn f() { return; } print f();"), lines(&["nil"]), "bare return gives nil");
    assert_eq!(out("fn g() { let a = 1; } print g();"), lines(&["nil"]), "no return gives nil");
    assert_eq!(out("fn h() { } h(); print 1;"), lines(&["1"]), "an empty function");
}

#[test]
fn return_stops_the_function() {
    assert_eq!(out("fn f() { print 1; return 2; print 3; } print f();"), lines(&["1", "2"]), "statements after return are skipped");
    assert_eq!(out("fn f(n) { if n > 0 { return 1; } return 0; } print f(5); print f(0 - 5);"), lines(&["1", "0"]), "return from inside an if");
    assert_eq!(
        out("fn first_over(limit) { let i = 0; while true { if i * i > limit { return i; } i = i + 1; } } print first_over(50);"),
        lines(&["8"]),
        "return from inside a loop"
    );
}

#[test]
fn recursion() {
    assert_eq!(out("fn fact(n) { if n <= 1 { return 1; } return n * fact(n - 1); } print fact(10);"), lines(&["3628800"]), "factorial");
    assert_eq!(out("fn fib(n) { if n < 2 { return n; } return fib(n - 1) + fib(n - 2); } print fib(15);"), lines(&["610"]), "fibonacci");
    assert_eq!(
        out("fn even(n) { if n == 0 { return true; } return odd(n - 1); } fn odd(n) { if n == 0 { return false; } return even(n - 1); } print even(10); print odd(7); print even(7);"),
        lines(&["true", "true", "false"]),
        "mutual recursion through the global scope"
    );
}

#[test]
fn functions_have_local_scope() {
    assert_eq!(
        run("fn f() { let hidden = 1; } f(); print hidden;"),
        Err(Error::Runtime(RuntimeError::Undefined("hidden".to_string()))),
        "locals do not leak"
    );
    assert_eq!(out("let x = 1; fn f(x) { x = x + 100; return x; } print f(5); print x;"), lines(&["105", "1"]), "a parameter shadows and assignment stays local");
    assert_eq!(out("let g = 1; fn f() { g = g + 1; } f(); f(); print g;"), lines(&["3"]), "assignment reaches a global");
}

#[test]
fn closures_remember_their_scope() {
    let adder = "fn make_adder(n) { fn add(x) { return x + n; } return add; }
let add5 = make_adder(5);
let add10 = make_adder(10);
print add5(1);
print add10(1);
print add5(add10(0));";
    assert_eq!(out(adder), lines(&["6", "11", "15"]), "each closure keeps its own n");
}

#[test]
fn closures_share_mutable_state() {
    let counter = "fn make_counter() {
    let n = 0;
    fn next() { n = n + 1; return n; }
    return next;
}
let a = make_counter();
let b = make_counter();
print a(); print a(); print a();
print b();";
    assert_eq!(out(counter), lines(&["1", "2", "3", "1"]), "state lives in the captured scope, one per make_counter call");
    let shared = "fn pair() {
    let v = 0;
    fn inc() { v = v + 1; }
    fn get() { return v; }
    fn both() { inc(); return get; }
    return both;
}
let both = pair();
let get = both();
print get();
get = both();
print get();";
    assert_eq!(out(shared), lines(&["1", "2"]), "functions defined in one scope see each other's changes");
}

#[test]
fn closures_see_later_changes() {
    assert_eq!(out("let x = 1; fn show() { print x; } x = 2; show();"), lines(&["2"]), "a closure captures the variable, not its value");
}

#[test]
fn calling_the_result_of_a_call() {
    assert_eq!(out("fn outer(a) { fn inner(b) { return a * b; } return inner; } print outer(6)(7);"), lines(&["42"]), "f(1)(2)");
}

#[test]
fn functions_are_values() {
    assert_eq!(out("fn f() { } print f;"), lines(&["<fn f>"]), "a function prints as <fn name>");
    assert_eq!(out("fn f(x) { return x + 1; } let g = f; print g(1);"), lines(&["2"]), "assign a function to a variable");
    assert_eq!(out("fn twice(f, x) { return f(f(x)); } fn inc(n) { return n + 1; } print twice(inc, 5);"), lines(&["7"]), "pass a function as an argument");
}

#[test]
fn call_errors() {
    assert_eq!(run("let x = 1; x(2);"), Err(Error::Runtime(RuntimeError::NotCallable)), "calling an int");
    assert_eq!(run("nosuch(1);"), Err(Error::Runtime(RuntimeError::Undefined("nosuch".to_string()))), "the callee is evaluated first");
    assert_eq!(
        run("fn f(a, b) { } f(1);"),
        Err(Error::Runtime(RuntimeError::Arity { expected: 2, got: 1 })),
        "too few arguments"
    );
    assert_eq!(
        run("fn f() { } f(1, 2, 3);"),
        Err(Error::Runtime(RuntimeError::Arity { expected: 0, got: 3 })),
        "too many arguments"
    );
    assert_eq!(
        run("fn f(a) { } f(1 / 0);"),
        Err(Error::Runtime(RuntimeError::DivByZero)),
        "argument errors come before the arity check"
    );
}

#[test]
fn errors_inside_functions_surface() {
    assert_eq!(run("fn f(n) { return 10 / n; } print f(0);"), Err(Error::Runtime(RuntimeError::DivByZero)), "division by zero inside a call");
}

#[test]
fn output_from_inside_functions_is_in_order() {
    assert_eq!(
        out("fn say(n) { print n; return n; } print say(1) + say(2);"),
        lines(&["1", "2", "3"]),
        "operands evaluate left to right, and prints inside calls come first"
    );
}

#[test]
fn parsing_functions() {
    assert_eq!(
        parse_program("fn f(a, b) { return a; }"),
        Ok(vec![Stmt::Fn(
            "f".to_string(),
            vec!["a".to_string(), "b".to_string()],
            vec![Stmt::Return(Some(Expr::Var("a".to_string())))]
        )]),
        "function with parameters"
    );
    assert_eq!(parse_program("fn g() { return; }"), Ok(vec![Stmt::Fn("g".to_string(), vec![], vec![Stmt::Return(None)])]), "bare return");
    let perr = |src: &str| match parse_program(src) {
        Err(e) => (e.msg, e.pos),
        Ok(p) => panic!("{:?} should not parse, got {:?}", src, p),
    };
    assert_eq!(perr("fn (a) { }"), ("expected identifier, found LParen".to_string(), 3), "function needs a name");
    assert_eq!(perr("fn f(a b) { }"), ("expected RParen, found Ident(\"b\")".to_string(), 7), "parameters need commas");
    assert_eq!(perr("fn f(a,) { }"), ("expected identifier, found RParen".to_string(), 7), "no trailing comma in parameters");
    assert_eq!(perr("fn f() return 1;"), ("expected LBrace, found Return".to_string(), 7), "body needs braces");
    assert_eq!(perr("fn f() { return 1 }"), ("expected Semi, found RBrace".to_string(), 18), "return needs a semicolon");
}

#[test]
fn top_level_return_ends_the_program() {
    assert_eq!(out("print 1; return; print 2;"), lines(&["1"]), "return outside a function stops the run");
}
