use std::cell::RefCell;
use std::rc::Rc;

type Log = Rc<RefCell<Vec<String>>>;

struct Guard {
    name: String,
    log: Log,
}

impl Guard {
    fn new(name: &str, log: &Log) -> Guard {
        Guard { name: name.to_string(), log: Rc::clone(log) }
    }
}

impl Drop for Guard {
    fn drop(&mut self) {
        self.log.borrow_mut().push(format!("drop {}", self.name));
    }
}

fn clone_all(items: &[String]) -> Vec<String> {
    items.to_vec()
}

fn with_added(mut v: Vec<String>, extra: String) -> Vec<String> {
    v.push(extra);
    v
}

fn run_scope(log: &Log, first: &str, second: &str) {
    let a = Guard::new(first, log);
    let _b = Guard::new(second, log);
    drop(a);
    log.borrow_mut().push("work".to_string());
}

fn main() {
    let log: Log = Rc::new(RefCell::new(Vec::new()));
    run_scope(&log, "a", "b");
    println!("{:?}", log.borrow());
}
