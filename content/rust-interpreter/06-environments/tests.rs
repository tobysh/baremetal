fn int(n: i64) -> Option<Value> {
    Some(Value::Int(n))
}

fn ev(src: &str, env: &Env) -> Result<Value, RuntimeError> {
    eval(&parse_expression(src).unwrap(), env)
}

#[test]
fn define_and_get() {
    let env = Env::new();
    assert_eq!(env.get("x"), None, "nothing defined yet");
    env.define("x", Value::Int(1));
    assert_eq!(env.get("x"), int(1), "defined");
    env.define("x", Value::Int(2));
    assert_eq!(env.get("x"), int(2), "define again overwrites");
    env.define("flag", Value::Bool(true));
    assert_eq!(env.get("flag"), Some(Value::Bool(true)), "other kinds of value");
    assert_eq!(env.get("y"), None, "unrelated name still missing");
}

#[test]
fn child_sees_parent() {
    let global = Env::new();
    global.define("a", Value::Int(1));
    let inner = global.child();
    assert_eq!(inner.get("a"), int(1), "lookup falls through to the parent");
    let innermost = inner.child();
    assert_eq!(innermost.get("a"), int(1), "and through grandparents");
    assert_eq!(innermost.get("zzz"), None, "missing everywhere");
}

#[test]
fn parent_does_not_see_child() {
    let global = Env::new();
    let inner = global.child();
    inner.define("local", Value::Int(5));
    assert_eq!(inner.get("local"), int(5), "visible inside");
    assert_eq!(global.get("local"), None, "invisible outside");
}

#[test]
fn shadowing() {
    let global = Env::new();
    global.define("x", Value::Int(1));
    let inner = global.child();
    inner.define("x", Value::Int(2));
    assert_eq!(inner.get("x"), int(2), "inner definition wins");
    assert_eq!(global.get("x"), int(1), "outer value untouched");
    let innermost = inner.child();
    assert_eq!(innermost.get("x"), int(2), "nearest definition wins");
}

#[test]
fn assign_updates_nearest() {
    let global = Env::new();
    global.define("x", Value::Int(1));
    let inner = global.child();
    assert!(inner.assign("x", Value::Int(9)), "assign finds the outer x");
    assert_eq!(global.get("x"), int(9), "the outer variable changed");
    inner.define("x", Value::Int(3));
    assert!(inner.assign("x", Value::Int(4)), "assign finds the inner x");
    assert_eq!(inner.get("x"), int(4), "inner changed");
    assert_eq!(global.get("x"), int(9), "outer not touched when shadowed");
}

#[test]
fn assign_unknown_is_false() {
    let env = Env::new();
    assert!(!env.assign("nope", Value::Int(1)), "assigning a name that does not exist fails");
    assert_eq!(env.get("nope"), None, "and does not create it");
    let inner = env.child();
    assert!(!inner.assign("nope", Value::Int(1)), "also from a child");
    assert_eq!(inner.get("nope"), None, "nothing created");
}

#[test]
fn clones_share_the_scope() {
    let env = Env::new();
    let alias = env.clone();
    alias.define("shared", Value::Int(7));
    assert_eq!(env.get("shared"), int(7), "a clone is another handle on the same scope");
    assert_eq!(env, alias, "equal handles");
    assert!(env != Env::new(), "different scopes are not equal");
    assert!(env.child() != env, "a child is a different scope");
}

#[test]
fn a_scope_can_outlive_its_creator() {
    let kept;
    {
        let global = Env::new();
        global.define("n", Value::Int(41));
        kept = global.child();
    }
    assert_eq!(kept.get("n"), int(41), "the child keeps its parent alive");
}

#[test]
fn depth_counts_scopes() {
    let global = Env::new();
    assert_eq!(global.depth(), 1, "global alone");
    let a = global.child();
    let b = a.child();
    assert_eq!(a.depth(), 2, "one level down");
    assert_eq!(b.depth(), 3, "two levels down");
    assert_eq!(global.child().depth(), 2, "siblings are the same depth");
    assert_eq!(global.depth(), 1, "parent unaffected");
}

#[test]
fn many_names_in_one_scope() {
    let env = Env::new();
    for i in 0..50 {
        env.define(&format!("v{}", i), Value::Int(i));
    }
    for i in 0..50 {
        assert_eq!(env.get(&format!("v{}", i)), int(i), "variable v{}", i);
    }
}

#[test]
fn output_is_shared_by_all_scopes() {
    let global = Env::new();
    let inner = global.child();
    global.emit("one".to_string());
    inner.emit("two".to_string());
    assert_eq!(global.take_output(), vec!["one".to_string(), "two".to_string()], "one buffer for the whole tree of scopes");
    assert_eq!(inner.take_output(), Vec::<String>::new(), "taking clears it");
    let other = Env::new();
    assert_eq!(other.take_output(), Vec::<String>::new(), "separate programs have separate buffers");
}

#[test]
fn eval_reads_variables() {
    let env = Env::new();
    env.define("x", Value::Int(6));
    env.define("y", Value::Int(7));
    assert_eq!(ev("x * y", &env), Ok(Value::Int(42)), "two variables");
    assert_eq!(ev("x < y", &env), Ok(Value::Bool(true)), "comparison");
    assert_eq!(ev("z", &env), Err(RuntimeError::Undefined("z".to_string())), "undefined variable");
    let inner = env.child();
    inner.define("x", Value::Int(100));
    assert_eq!(ev("x + y", &inner), Ok(Value::Int(107)), "inner x shadows, y from outside");
    assert_eq!(ev("x + y", &env), Ok(Value::Int(13)), "outer scope unaffected");
}
