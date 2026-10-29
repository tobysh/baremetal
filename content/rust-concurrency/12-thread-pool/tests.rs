use std::collections::HashSet;
use std::sync::Barrier;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;

#[test]
fn reports_size() {
    assert_eq!(ThreadPool::new(4).size(), 4);
    assert_eq!(ThreadPool::new(1).size(), 1);
}

#[test]
#[should_panic]
fn zero_workers_is_rejected() {
    let _ = ThreadPool::new(0);
}

#[test]
fn drop_waits_for_all_jobs() {
    let count = Arc::new(AtomicUsize::new(0));
    {
        let pool = ThreadPool::new(4);
        for _ in 0..50 {
            let c = Arc::clone(&count);
            pool.execute(move || {
                std::thread::sleep(Duration::from_millis(2));
                c.fetch_add(1, Ordering::SeqCst);
            });
        }
    }
    assert_eq!(count.load(Ordering::SeqCst), 50, "dropping the pool must wait for every queued job");
}

#[test]
fn jobs_run_on_worker_threads_not_the_caller() {
    let ids = Arc::new(Mutex::new(HashSet::new()));
    let me = std::thread::current().id();
    {
        let pool = ThreadPool::new(3);
        for _ in 0..30 {
            let ids = Arc::clone(&ids);
            pool.execute(move || {
                ids.lock().unwrap().insert(std::thread::current().id());
                std::thread::sleep(Duration::from_millis(1));
            });
        }
    }
    let ids = ids.lock().unwrap();
    assert!(!ids.contains(&me), "no job may run on the calling thread");
    assert!(!ids.is_empty() && ids.len() <= 3, "jobs should run on at most 3 workers, saw {}", ids.len());
}

#[test]
fn execute_does_not_block() {
    let (returned_tx, returned_rx) = mpsc::channel();
    let (release_tx, release_rx) = mpsc::channel();
    let owner = std::thread::spawn(move || {
        let pool = ThreadPool::new(1);
        pool.execute(move || { let _ = release_rx.recv(); });
        returned_tx.send(()).unwrap();
    });
    let returned = returned_rx.recv_timeout(Duration::from_secs(2));
    // Always release before asserting: even a synchronous implementation can unwind.
    let _ = release_tx.send(());
    owner.join().unwrap();
    assert!(returned.is_ok(), "execute must return while the submitted job is still blocked");
}

#[test]
fn jobs_run_concurrently() {
    // Two jobs that each wait for the other can only finish if they run at the same time.
    let barrier = Arc::new(Barrier::new(2));
    let done = Arc::new(AtomicUsize::new(0));
    {
        let pool = ThreadPool::new(2);
        for _ in 0..2 {
            let b = Arc::clone(&barrier);
            let d = Arc::clone(&done);
            pool.execute(move || {
                b.wait();
                d.fetch_add(1, Ordering::SeqCst);
            });
        }
    }
    assert_eq!(done.load(Ordering::SeqCst), 2, "two workers must be able to run jobs simultaneously");
}

#[test]
fn results_can_flow_back_over_a_channel() {
    let (tx, rx) = mpsc::channel();
    {
        let pool = ThreadPool::new(3);
        for i in 0..10u32 {
            let tx = tx.clone();
            pool.execute(move || tx.send(i * i).unwrap());
        }
    }
    drop(tx);
    let mut got: Vec<u32> = rx.iter().collect();
    got.sort();
    assert_eq!(got, (0..10u32).map(|i| i * i).collect::<Vec<_>>());
}
