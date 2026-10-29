fn p(src: &str) -> String {
    match parse_expression(src) {
        Ok(e) => show(&e),
        Err(e) => panic!("{:?} should parse but gave {:?}", src, e),
    }
}

fn perr(src: &str) -> (String, usize) {
    match parse_expression(src) {
        Ok(e) => panic!("{:?} should not parse but gave {}", src, show(&e)),
        Err(e) => (e.msg, e.pos),
    }
}

#[test]
fn atoms() {
    assert_eq!(p("42"), "42", "number");
    assert_eq!(p("true"), "true", "true");
    assert_eq!(p("false"), "false", "false");
    assert_eq!(p("width"), "width", "variable");
    assert_eq!(p("  7  "), "7", "whitespace around");
}

#[test]
fn tree_shape() {
    assert_eq!(
        parse_expression("1 + x"),
        Ok(Expr::Binary(BinOp::Add, Box::new(Expr::Num(1)), Box::new(Expr::Var("x".to_string())))),
        "the tree itself, not just its text"
    );
    assert_eq!(parse_expression("-a"), Ok(Expr::Unary(UnOp::Neg, Box::new(Expr::Var("a".to_string())))), "unary node");
}

#[test]
fn multiplication_binds_tighter_than_addition() {
    assert_eq!(p("1 + 2 * 3"), "(+ 1 (* 2 3))", "2 * 3 first");
    assert_eq!(p("1 * 2 + 3"), "(+ (* 1 2) 3)", "1 * 2 first");
    assert_eq!(p("1 - 2 / 3"), "(- 1 (/ 2 3))", "division before subtraction");
    assert_eq!(p("a % b + c"), "(+ (% a b) c)", "remainder is a multiplication-level operator");
}

#[test]
fn left_associativity() {
    assert_eq!(p("1 - 2 - 3"), "(- (- 1 2) 3)", "subtraction groups to the left");
    assert_eq!(p("8 / 4 / 2"), "(/ (/ 8 4) 2)", "division groups to the left");
    assert_eq!(p("1 + 2 + 3 + 4"), "(+ (+ (+ 1 2) 3) 4)", "a long chain");
    assert_eq!(p("2 * 3 % 4"), "(% (* 2 3) 4)", "same level groups left");
    assert_eq!(p("a < b < c"), "(< (< a b) c)", "comparisons too");
}

#[test]
fn comparison_is_loosest() {
    assert_eq!(p("1 + 2 < 3 * 4"), "(< (+ 1 2) (* 3 4))", "arithmetic first");
    assert_eq!(p("a == b + 1"), "(== a (+ b 1))", "== is looser than +");
    assert_eq!(p("a != b"), "(!= a b)", "!=");
    assert_eq!(p("a <= b"), "(<= a b)", "<=");
    assert_eq!(p("a >= b"), "(>= a b)", ">=");
    assert_eq!(p("a > b"), "(> a b)", ">");
    assert_eq!(p("x < y == true"), "(== (< x y) true)", "chain of comparisons groups left");
}

#[test]
fn parentheses_override() {
    assert_eq!(p("(1 + 2) * 3"), "(* (+ 1 2) 3)", "parens first");
    assert_eq!(p("2 * (3 + 4)"), "(* 2 (+ 3 4))", "on the right");
    assert_eq!(p("1 - (2 - 3)"), "(- 1 (- 2 3))", "forces right grouping");
    assert_eq!(p("((7))"), "7", "nested parens vanish");
    assert_eq!(p("(a)"), "a", "parenthesised variable");
}

#[test]
fn unary_operators() {
    assert_eq!(p("-5"), "(- 5)", "negation");
    assert_eq!(p("!done"), "(! done)", "not");
    assert_eq!(p("--3"), "(- (- 3))", "double negation");
    assert_eq!(p("!!a"), "(! (! a))", "double not");
    assert_eq!(p("-2 * 3"), "(* (- 2) 3)", "unary binds tighter than *");
    assert_eq!(p("2 * -3"), "(* 2 (- 3))", "unary on the right operand");
    assert_eq!(p("1 - -1"), "(- 1 (- 1))", "binary then unary minus");
    assert_eq!(p("-(1 + 2)"), "(- (+ 1 2))", "negating a group");
    assert_eq!(p("!a == b"), "(== (! a) b)", "not binds tighter than ==");
    assert_eq!(p("-a + b"), "(+ (- a) b)", "prefix first");
}

#[test]
fn calls() {
    assert_eq!(p("f()"), "(call f)", "no arguments");
    assert_eq!(p("f(1)"), "(call f 1)", "one argument");
    assert_eq!(p("f(1, 2 + 3, x)"), "(call f 1 (+ 2 3) x)", "several arguments");
    assert_eq!(p("f(g(1), h())"), "(call f (call g 1) (call h))", "nested calls");
    assert_eq!(p("f(1)(2)"), "(call (call f 1) 2)", "a call on the result of a call");
    assert_eq!(p("(f)(1)"), "(call f 1)", "callee in parens");
}

#[test]
fn calls_bind_tightest() {
    assert_eq!(p("-f(1)"), "(- (call f 1))", "call before unary minus");
    assert_eq!(p("1 + f(2) * 3"), "(+ 1 (* (call f 2) 3))", "call inside arithmetic");
    assert_eq!(p("!ok(x)"), "(! (call ok x))", "call before not");
    assert_eq!(p("f(1) < g(2)"), "(< (call f 1) (call g 2))", "calls either side of a comparison");
}

#[test]
fn bigger_expressions() {
    assert_eq!(p("a + b * c - d / e"), "(- (+ a (* b c)) (/ d e))", "mixed");
    assert_eq!(p("(a + b) * (c - d) % 7"), "(% (* (+ a b) (- c d)) 7)", "grouped");
    assert_eq!(p("1 < 2 == 2 > 1"), "(> (== (< 1 2) 2) 1)", "left grouping at one level");
    assert_eq!(p("(1 < 2) == (2 > 1)"), "(== (< 1 2) (> 2 1))", "parentheses explicitly compare the two booleans");
}

#[test]
fn errors_at_end_of_input() {
    assert_eq!(perr(""), ("unexpected end of input".to_string(), 0), "nothing at all");
    assert_eq!(perr("1 +"), ("unexpected end of input".to_string(), 3), "operator without right side");
    assert_eq!(perr("-"), ("unexpected end of input".to_string(), 1), "dangling minus");
    assert_eq!(perr("(1 + 2"), ("expected RParen, found end of input".to_string(), 6), "unclosed paren");
    assert_eq!(perr("f(1, 2"), ("expected RParen, found end of input".to_string(), 6), "unclosed call");
    assert_eq!(perr("f(1,"), ("unexpected end of input".to_string(), 4), "argument expected after comma");
}

#[test]
fn errors_in_the_middle() {
    assert_eq!(perr(")"), ("unexpected RParen".to_string(), 0), "stray close paren");
    assert_eq!(perr("* 3"), ("unexpected Star".to_string(), 0), "operator where an operand belongs");
    assert_eq!(perr("1 + * 2"), ("unexpected Star".to_string(), 4), "two operators in a row");
    assert_eq!(perr("f(,)"), ("unexpected Comma".to_string(), 2), "empty argument");
    assert_eq!(perr("f(1,)"), ("unexpected RParen".to_string(), 4), "trailing comma is not allowed");
    assert_eq!(perr("(1 2)"), ("expected RParen, found Num(2)".to_string(), 3), "missing operator inside parens");
}

#[test]
fn leftover_tokens_are_an_error() {
    assert_eq!(perr("1 2"), ("unexpected Num(2)".to_string(), 2), "two numbers in a row");
    assert_eq!(perr("a b c"), ("unexpected Ident(\"b\")".to_string(), 2), "first leftover reported");
    assert_eq!(perr("1 + 2 )"), ("unexpected RParen".to_string(), 6), "extra close paren");
}

#[test]
fn lex_errors_come_through() {
    assert_eq!(perr("1 + $"), ("unexpected character '$'".to_string(), 4), "lexer error converted");
    assert_eq!(perr("99999999999999999999"), ("number too big".to_string(), 0), "number too big");
}

#[test]
fn deep_nesting() {
    let src = format!("{}1{}", "(".repeat(100), ")".repeat(100));
    assert_eq!(p(&src), "1", "one hundred nested groups");
    let chain = vec!["1"; 200].join(" + ");
    assert!(p(&chain).starts_with("(+ (+ "), "a long left-leaning chain");
}
