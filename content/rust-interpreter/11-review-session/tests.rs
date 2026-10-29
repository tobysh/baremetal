#[test]
fn persistent_definitions_and_expression_echo() {
    let mut s = Session::new();
    assert_eq!(s.ok_lines(), 0);
    assert_eq!(s.eval_line("let x = 4;"), "");
    assert_eq!(s.eval_line("x + 3"), "7");
    assert_eq!(s.eval_line("x = x * 2;"), "");
    assert_eq!(s.eval_line("x"), "8");
    assert_eq!(s.eval_line("x + 1;"), "", "semicolon discards the result");
    assert_eq!(s.ok_lines(), 5);
    let mut other = Session::new();
    assert_eq!(other.eval_line("x"), "runtime error: undefined variable 'x'");
    assert_eq!(other.ok_lines(), 0);
}

#[test]
fn blanks_comments_and_counter_rules() {
    let mut s = Session::new();
    for line in ["", "   ", "\t\n"] { assert_eq!(s.eval_line(line), ""); }
    assert_eq!(s.ok_lines(), 0);
    assert_eq!(s.eval_line("# a comment"), "");
    assert_eq!(s.ok_lines(), 1);
    assert_eq!(s.eval_line("print 1; print 2;"), "1\n2");
    assert_eq!(s.ok_lines(), 2, "counts inputs, not statements");
    assert_eq!(s.eval_line("1 / 0"), "runtime error: division by zero");
    assert_eq!(s.ok_lines(), 2);
}

#[test]
fn prints_are_drained_on_success_and_error() {
    let mut s = Session::new();
    assert_eq!(s.eval_line("fn f(n) { print n; return n + 1; }"), "");
    for n in [0, 3, -7] {
        assert_eq!(s.eval_line(&format!("f({n})")), format!("{n}\n{}", n+1));
        assert_eq!(s.eval_line("true"), "true", "old output must be cleared");
    }
    assert_eq!(s.eval_line("print 8; print 1 / 0;"), "8\nruntime error: division by zero");
    assert_eq!(s.eval_line("false"), "false");
}

#[test]
fn failures_keep_prior_mutations_and_do_not_reparse_runtime_errors() {
    let mut s = Session::new();
    assert_eq!(s.eval_line("let x = 1;"), "");
    assert_eq!(s.eval_line("x = 9; print missing;"), "runtime error: undefined variable 'missing'");
    assert_eq!(s.eval_line("x"), "9");
    assert!(s.eval_line("x = 20; print ;").starts_with("error: unexpected Semi"));
    assert_eq!(s.eval_line("x"), "9", "parse the whole program before execution");
    assert_eq!(s.eval_line("fn fail() { x = x + 1; return 1 / 0; }"), "");
    assert_eq!(s.eval_line("fail()"), "runtime error: division by zero");
    assert_eq!(s.eval_line("x"), "10", "runtime error must not rerun an expression as a program");
}

#[test]
fn returned_closures_survive_across_prompts() {
    let mut s = Session::new();
    assert_eq!(s.eval_line("fn counter(n) { fn step() { n = n + 1; return n; } return step; }"), "");
    assert_eq!(s.eval_line("let a = counter(10); let b = counter(30);"), "");
    for (line, want) in [("a()", "11"), ("a()", "12"), ("b()", "31"), ("a()", "13")] {
        assert_eq!(s.eval_line(line), want);
    }
}

#[test]
fn transcripts_have_one_prompt_per_input() {
    assert_eq!(transcript(&[]), "");
    assert_eq!(transcript(&["let x = 2;", "x * 3", "", "print x;", "1 / 0", "x"]),
        "> let x = 2;\n> x * 3\n6\n> \n> print x;\n2\n> 1 / 0\nruntime error: division by zero\n> x\n2");
    assert_eq!(transcript(&["false"]), "> false\nfalse", "a fresh session per transcript");
}
