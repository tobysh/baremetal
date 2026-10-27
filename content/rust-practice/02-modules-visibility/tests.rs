use crate::counter::Counter;

#[test]
fn starts_at_zero() {
    let c = Counter::new(3);
    assert_eq!(c.count(), 0, "a new counter starts at 0");
}

#[test]
fn tick_adds_the_step() {
    let mut c = Counter::new(3);
    assert_eq!(c.tick(), 3, "first tick returns 3");
    assert_eq!(c.tick(), 6, "second tick returns 6");
    assert_eq!(c.count(), 6, "count() agrees with the last tick");
}

#[test]
fn zero_step_becomes_one() {
    let mut c = Counter::new(0);
    assert_eq!(c.tick(), 1, "a step of 0 is treated as 1");
    assert_eq!(c.tick(), 2, "and keeps counting");
}

#[test]
fn reset_keeps_the_step() {
    let mut c = Counter::new(4);
    c.tick();
    c.reset();
    assert_eq!(c.count(), 0, "reset returns to 0");
    assert_eq!(c.tick(), 4, "the step is still 4");
}

#[test]
fn saturates_instead_of_overflowing() {
    let mut c = Counter::new(u32::MAX);
    assert_eq!(c.tick(), u32::MAX, "first tick reaches the maximum");
    assert_eq!(c.tick(), u32::MAX, "second tick stays at the maximum");
}

#[test]
fn label_formats_the_count() {
    let mut c = Counter::new(1);
    c.tick();
    c.tick();
    c.tick();
    assert_eq!(crate::counter::format::label(&c), "count=3", "label should be count=3");
}

#[test]
fn label_of_fresh_counter() {
    let c = Counter::new(9);
    assert_eq!(crate::counter::format::label(&c), "count=0", "label should be count=0");
}
