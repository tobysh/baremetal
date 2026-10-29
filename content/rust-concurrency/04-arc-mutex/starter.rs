use std::sync::{Arc, Mutex};
use std::thread;

fn count_up(threads: usize, per: usize) -> u64 {
    0
}

struct Account {
    balance: Arc<Mutex<i64>>,
}

impl Account {
    fn new(balance: i64) -> Account {
        Account { balance: Arc::new(Mutex::new(balance)) }
    }

    fn handle(&self) -> Account {
        // Should share the same balance, not start a new one.
        Account::new(0)
    }

    fn deposit(&self, n: i64) {
    }

    fn withdraw(&self, n: i64) -> bool {
        false
    }

    fn balance(&self) -> i64 {
        *self.balance.lock().unwrap()
    }
}

fn lock_even_if_poisoned(m: &Mutex<i32>) -> i32 {
    // Panics if the mutex is poisoned. Fix that.
    *m.lock().unwrap()
}

// Try it with Run.
fn main() {
    println!("{}", count_up(4, 1000));
    let a = Account::new(100);
    println!("{} {}", a.withdraw(30), a.balance());
}
