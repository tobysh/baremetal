#[test]
fn counts_without_losing_updates() {
    assert_eq!(count_up(4, 1000), 4000, "4 x 1000 increments");
    assert_eq!(count_up(8, 5000), 40_000, "8 x 5000 increments");
}

#[test]
fn counts_edge_cases() {
    assert_eq!(count_up(0, 100), 0, "no threads");
    assert_eq!(count_up(3, 0), 0, "no increments");
    assert_eq!(count_up(1, 7), 7, "single thread");
}

#[test]
fn account_basic() {
    let a = Account::new(100);
    assert!(a.withdraw(30), "30 of 100 is fine");
    assert_eq!(a.balance(), 70);
    assert!(!a.withdraw(71), "71 of 70 must be refused");
    assert_eq!(a.balance(), 70, "a refused withdrawal changes nothing");
    a.deposit(5);
    assert_eq!(a.balance(), 75);
    assert!(a.withdraw(75), "withdrawing exactly the balance is allowed");
    assert_eq!(a.balance(), 0);
}

#[test]
fn handles_share_the_balance() {
    let a = Account::new(10);
    let b = a.handle();
    b.deposit(5);
    assert_eq!(a.balance(), 15, "a deposit through a handle must be visible on the original");
    assert!(a.withdraw(15));
    assert_eq!(b.balance(), 0, "and the other way round");
}

#[test]
fn concurrent_withdrawals_never_overdraw() {
    let a = Account::new(100);
    let handles: Vec<_> = (0..16)
        .map(|_| {
            let acc = a.handle();
            std::thread::spawn(move || {
                let mut ok = 0;
                for _ in 0..10 {
                    if acc.withdraw(1) { ok += 1; }
                }
                ok
            })
        })
        .collect();
    let granted: i32 = handles.into_iter().map(|h| h.join().unwrap()).sum();
    assert_eq!(granted, 100, "exactly 100 one-unit withdrawals can succeed");
    assert_eq!(a.balance(), 0, "the balance ends at 0, never negative");
}

#[test]
fn concurrent_deposits_add_up() {
    let a = Account::new(0);
    let handles: Vec<_> = (0..8)
        .map(|_| {
            let acc = a.handle();
            std::thread::spawn(move || { for _ in 0..500 { acc.deposit(2); } })
        })
        .collect();
    for h in handles { h.join().unwrap(); }
    assert_eq!(a.balance(), 8000, "8 threads x 500 deposits x 2");
}

#[test]
fn survives_poison() {
    let m = Arc::new(Mutex::new(41));
    let m2 = Arc::clone(&m);
    let r = std::thread::spawn(move || {
        let mut g = m2.lock().unwrap();
        *g += 1;
        panic!("die holding the lock");
    })
    .join();
    assert!(r.is_err(), "the helper thread should have panicked");
    assert_eq!(lock_even_if_poisoned(&m), 42, "the value written before the panic is still readable");
}

#[test]
fn unpoisoned_still_works() {
    let m = Mutex::new(5);
    assert_eq!(lock_even_if_poisoned(&m), 5, "a healthy mutex just returns its value");
}
