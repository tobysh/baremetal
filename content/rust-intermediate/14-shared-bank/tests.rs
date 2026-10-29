use std::sync::Barrier;

#[test]
fn new_balance_total() {
    let b = Bank::new(&[100, 50, 7]);
    assert_eq!(b.balance(0), Some(100), "account 0");
    assert_eq!(b.balance(2), Some(7), "account 2");
    assert_eq!(b.balance(3), None, "no account 3");
    assert_eq!(b.total(), 157, "total");
    assert_eq!(Bank::new(&[]).total(), 0, "an empty bank");
}

#[test]
fn deposit_cases() {
    let b = Bank::new(&[10, 20]);
    assert_eq!(b.deposit(1, 5), Ok(()), "deposit");
    assert_eq!(b.deposit(0, -3), Ok(()), "a negative deposit is a withdrawal");
    assert_eq!((b.balance(0), b.balance(1)), (Some(7), Some(25)), "balances");
    assert_eq!(b.deposit(9, 1), Err("no such account".to_string()), "bad index");
}

#[test]
fn transfer_moves_money_both_directions() {
    let b = Bank::new(&[100, 50, 0]);
    assert_eq!(b.transfer(0, 1, 30), Ok(()), "0 -> 1");
    assert_eq!(b.transfer(1, 0, 10), Ok(()), "1 -> 0");
    assert_eq!(b.transfer(2, 0, 0), Err("invalid amount".to_string()), "zero is invalid");
    assert_eq!((b.balance(0), b.balance(1), b.balance(2)), (Some(80), Some(70), Some(0)), "balances after transfers");
    assert_eq!(b.total(), 150, "money is conserved");
}

#[test]
fn transfer_errors_in_order() {
    let b = Bank::new(&[10, 10]);
    assert_eq!(b.transfer(0, 5, 1), Err("no such account".to_string()), "bad destination");
    assert_eq!(b.transfer(7, 0, 1), Err("no such account".to_string()), "bad source");
    assert_eq!(b.transfer(1, 1, 1), Err("same account".to_string()), "same account");
    assert_eq!(b.transfer(1, 1, -5), Err("same account".to_string()), "same account before amount");
    assert_eq!(b.transfer(0, 1, -5), Err("invalid amount".to_string()), "negative amount");
    assert_eq!(b.transfer(0, 1, 11), Err("insufficient funds".to_string()), "too much");
    assert_eq!(b.transfer(1, 0, 10), Ok(()), "exactly all of it");
    assert_eq!((b.balance(0), b.balance(1)), (Some(20), Some(0)), "failed transfers change nothing");
}

#[test]
fn parallel_deposits_all_arrive() {
    let bank = Arc::new(Bank::new(&[0, 1000]));
    let amounts: Vec<i64> = (1..=20).collect();
    parallel_deposits(&bank, 0, amounts);
    assert_eq!(bank.balance(0), Some(210), "1 + 2 + ... + 20");
    assert_eq!(bank.balance(1), Some(1000), "the other account is untouched");
    parallel_deposits(&bank, 1, vec![]);
    assert_eq!(bank.total(), 1210, "an empty list changes nothing");
}

#[test]
fn opposite_transfers_do_not_deadlock_or_lose_money() {
    let bank = Arc::new(Bank::new(&[100_000, 100_000]));
    let start = Arc::new(Barrier::new(4));
    let mut handles = Vec::new();
    for t in 0..4 {
        let bank = Arc::clone(&bank);
        let start = Arc::clone(&start);
        handles.push(thread::spawn(move || {
            start.wait();
            for _ in 0..2000 {
                if t % 2 == 0 {
                    bank.transfer(0, 1, 1).unwrap();
                } else {
                    bank.transfer(1, 0, 1).unwrap();
                }
            }
        }));
    }
    for h in handles {
        h.join().unwrap();
    }
    assert_eq!(bank.balance(0), Some(100_000), "equal and opposite transfers cancel out");
    assert_eq!(bank.balance(1), Some(100_000), "account 1 as well");
}

#[test]
fn a_ring_of_transfers_conserves_money() {
    let bank = Arc::new(Bank::new(&[500, 500, 500]));
    let start = Arc::new(Barrier::new(3));
    let mut handles = Vec::new();
    for t in 0..3usize {
        let bank = Arc::clone(&bank);
        let start = Arc::clone(&start);
        handles.push(thread::spawn(move || {
            start.wait();
            let (a, b) = (t % 3, (t + 1) % 3);
            for _ in 0..1000 {
                bank.transfer(a, b, 1).ok();
            }
        }));
    }
    for h in handles {
        h.join().unwrap();
    }
    assert_eq!(bank.total(), 1500, "no money appears or disappears");
    for i in 0..3 {
        assert!(bank.balance(i).unwrap() >= 0, "no account goes negative");
    }
}
