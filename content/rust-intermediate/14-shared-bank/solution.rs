use std::sync::{Arc, Mutex};
use std::thread;

struct Bank {
    accounts: Vec<Mutex<i64>>,
}

impl Bank {
    fn new(balances: &[i64]) -> Bank {
        Bank { accounts: balances.iter().map(|&b| Mutex::new(b)).collect() }
    }

    fn balance(&self, i: usize) -> Option<i64> {
        self.accounts.get(i).map(|m| *m.lock().unwrap())
    }

    fn total(&self) -> i64 {
        self.accounts.iter().map(|m| *m.lock().unwrap()).sum()
    }

    fn deposit(&self, i: usize, amount: i64) -> Result<(), String> {
        let m = self.accounts.get(i).ok_or_else(|| "no such account".to_string())?;
        *m.lock().unwrap() += amount;
        Ok(())
    }

    fn transfer(&self, from: usize, to: usize, amount: i64) -> Result<(), String> {
        let n = self.accounts.len();
        if from >= n || to >= n {
            return Err("no such account".to_string());
        }
        if from == to {
            return Err("same account".to_string());
        }
        if amount <= 0 {
            return Err("invalid amount".to_string());
        }
        let (lo, hi) = if from < to { (from, to) } else { (to, from) };
        let mut first = self.accounts[lo].lock().unwrap();
        let mut second = self.accounts[hi].lock().unwrap();
        let (src, dst) = if from < to { (&mut *first, &mut *second) } else { (&mut *second, &mut *first) };
        if *src < amount {
            return Err("insufficient funds".to_string());
        }
        *src -= amount;
        *dst += amount;
        Ok(())
    }
}

fn parallel_deposits(bank: &Arc<Bank>, account: usize, amounts: Vec<i64>) {
    let handles: Vec<_> = amounts
        .into_iter()
        .map(|amt| {
            let b = Arc::clone(bank);
            thread::spawn(move || b.deposit(account, amt))
        })
        .collect();
    for h in handles {
        h.join().unwrap().ok();
    }
}

// Try it with Run.
fn main() {
    let bank = Arc::new(Bank::new(&[100, 50]));
    println!("{:?}", bank.transfer(0, 1, 30));
    parallel_deposits(&bank, 1, vec![1, 2, 3]);
    println!("{:?} {}", bank.balance(1), bank.total());
}
