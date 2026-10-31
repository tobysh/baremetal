use std::cell::Cell;

thread_local! {
    pub static LIVE: Cell<isize> = Cell::new(0);
}

pub struct Counter {
    step: u32,
    total: u64,
}

impl Counter {
    fn new(step: u32) -> Counter {
        LIVE.with(|l| l.set(l.get() + 1));
        Counter { step, total: 0 }
    }
}

impl Drop for Counter {
    fn drop(&mut self) {
        LIVE.with(|l| l.set(l.get() - 1));
    }
}

fn counter_new(step: u32) -> *mut Counter {
    std::ptr::null_mut()
}

unsafe fn counter_bump(c: *mut Counter) -> u64 {
    0
}

unsafe fn counter_total(c: *const Counter) -> u64 {
    0
}

unsafe fn counter_free(c: *mut Counter) {}

unsafe fn counter_into_inner(c: *mut Counter) -> u64 {
    0
}

// Try it with Run.
fn main() {
    let c = counter_new(3);
    unsafe {
        counter_bump(c);
        println!("{}", counter_total(c));
        counter_free(c);
    }
    println!("live: {}", LIVE.with(|l| l.get()));
}
