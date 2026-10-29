use std::sync::Arc;
use std::time::{Duration, Instant};

#[test]
fn fifo_order() {
    let q = BlockingQueue::new();
    for i in 0..5 { q.push(i); }
    assert_eq!(q.len(), 5);
    let got: Vec<i32> = (0..5).map(|_| q.pop()).collect();
    assert_eq!(got, vec![0, 1, 2, 3, 4], "first in, first out");
    assert_eq!(q.len(), 0);
}

#[test]
fn pop_waits_for_a_late_producer() {
    let q = Arc::new(BlockingQueue::new());
    let q2 = Arc::clone(&q);
    let producer = std::thread::spawn(move || {
        std::thread::sleep(Duration::from_millis(50));
        q2.push(String::from("late"));
    });
    let v = q.pop();
    assert_eq!(v, "late", "pop should sleep until the producer pushes");
    producer.join().unwrap();
}

#[test]
fn pop_timeout_expires_on_empty_queue() {
    let q: BlockingQueue<i32> = BlockingQueue::new();
    let t = Instant::now();
    let r = q.pop_timeout(Duration::from_millis(60));
    assert_eq!(r, None, "nothing arrived, so None");
    assert!(t.elapsed() >= Duration::from_millis(40), "it should actually wait for about the timeout, waited {:?}", t.elapsed());
}

#[test]
fn pop_timeout_returns_item_that_arrives_in_time() {
    let q = Arc::new(BlockingQueue::new());
    let q2 = Arc::clone(&q);
    let h = std::thread::spawn(move || {
        std::thread::sleep(Duration::from_millis(30));
        q2.push(7);
    });
    let r = q.pop_timeout(Duration::from_secs(3));
    assert_eq!(r, Some(7), "the item arrives well within the timeout");
    h.join().unwrap();
}

#[test]
fn pop_timeout_returns_immediately_when_item_waiting() {
    let q = BlockingQueue::new();
    q.push(1);
    let t = Instant::now();
    assert_eq!(q.pop_timeout(Duration::from_secs(5)), Some(1));
    assert!(t.elapsed() < Duration::from_secs(2), "must not wait when an item is already there");
}

#[test]
fn many_producers_many_consumers() {
    let q = Arc::new(BlockingQueue::new());
    let producers: Vec<_> = (0..4)
        .map(|p| {
            let q = Arc::clone(&q);
            std::thread::spawn(move || { for i in 0..100 { q.push(p * 1000 + i); } })
        })
        .collect();
    let consumers: Vec<_> = (0..4)
        .map(|_| {
            let q = Arc::clone(&q);
            std::thread::spawn(move || (0..100).map(|_| q.pop()).collect::<Vec<i32>>())
        })
        .collect();
    for p in producers { p.join().unwrap(); }
    let mut all: Vec<i32> = consumers.into_iter().flat_map(|c| c.join().unwrap()).collect();
    all.sort();
    let mut expected: Vec<i32> = (0..4).flat_map(|p| (0..100).map(move |i| p * 1000 + i)).collect();
    expected.sort();
    assert_eq!(all, expected, "every pushed item is popped exactly once");
    assert_eq!(q.len(), 0);
}
