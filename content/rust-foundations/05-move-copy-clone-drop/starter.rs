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

// TODO: implement Drop for Guard

fn clone_all(items: &[String]) -> Vec<String> {
    Vec::new()
}

fn with_added(mut v: Vec<String>, extra: String) -> Vec<String> {
    v
}

fn run_scope(log: &Log, first: &str, second: &str) {
    // your code here
}

// Try your code with Run.
fn main() {
    let log: Log = Rc::new(RefCell::new(Vec::new()));
    run_scope(&log, "a", "b");
    println!("{:?}", log.borrow());
}
