use std::sync::{Arc, Mutex};
use std::thread;

fn count_up(threads: usize, per: usize) -> u64 {
    let counter = Arc::new(Mutex::new(0u64));
    let handles: Vec<_> = (0..threads)
        .map(|_| {
            let counter = Arc::clone(&counter);
            thread::spawn(move || {
                for _ in 0..per {
                    *counter.lock().unwrap() += 1;
                }
            })
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }
    let total = *counter.lock().unwrap();
    total
}

struct Account {
    balance: Arc<Mutex<i64>>,
}

impl Account {
    fn new(balance: i64) -> Account {
        Account { balance: Arc::new(Mutex::new(balance)) }
    }

    fn handle(&self) -> Account {
        Account { balance: Arc::clone(&self.balance) }
    }

    fn deposit(&self, n: i64) {
        *self.balance.lock().unwrap() += n;
    }

    fn withdraw(&self, n: i64) -> bool {
        let mut g = self.balance.lock().unwrap();
        if *g >= n {
            *g -= n;
            true
        } else {
            false
        }
    }

    fn balance(&self) -> i64 {
        *self.balance.lock().unwrap()
    }
}

fn lock_even_if_poisoned(m: &Mutex<i32>) -> i32 {
    *m.lock().unwrap_or_else(|e| e.into_inner())
}

// Try it with Run.
fn main() {
    println!("{}", count_up(4, 1000));
    let a = Account::new(100);
    println!("{} {}", a.withdraw(30), a.balance());
}
