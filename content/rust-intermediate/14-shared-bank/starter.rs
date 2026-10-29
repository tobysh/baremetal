use std::sync::{Arc, Mutex};
use std::thread;

struct Bank {
    accounts: Vec<Mutex<i64>>,
}

impl Bank {
    fn new(balances: &[i64]) -> Bank {
        Bank { accounts: Vec::new() }
    }

    fn balance(&self, i: usize) -> Option<i64> {
        None
    }

    fn total(&self) -> i64 {
        0
    }

    fn deposit(&self, i: usize, amount: i64) -> Result<(), String> {
        Err(String::new())
    }

    fn transfer(&self, from: usize, to: usize, amount: i64) -> Result<(), String> {
        Err(String::new())
    }
}

fn parallel_deposits(bank: &Arc<Bank>, account: usize, amounts: Vec<i64>) {}

// Try it with Run.
fn main() {
    let bank = Arc::new(Bank::new(&[100, 50]));
    println!("{:?}", bank.transfer(0, 1, 30));
    parallel_deposits(&bank, 1, vec![1, 2, 3]);
    println!("{:?} {}", bank.balance(1), bank.total());
}
