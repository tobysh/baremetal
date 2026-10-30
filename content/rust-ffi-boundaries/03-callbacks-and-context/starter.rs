use std::ffi::c_void;

pub type Visit = extern "C" fn(ctx: *mut c_void, value: i32) -> i32;

#[derive(Debug, PartialEq)]
pub struct Stats {
    pub count: usize,
    pub sum: i64,
    pub max: Option<i32>,
}

#[derive(Debug, PartialEq)]
pub struct Search {
    pub seen: usize,
    pub found: Option<usize>,
}

/// Calls cb(ctx, x) for each element until cb returns non-zero.
/// Returns the number of calls, or -1 for a null `xs` with len > 0.
unsafe fn for_each(xs: *const i32, len: usize, cb: Visit, ctx: *mut c_void) -> isize {
    0
}

extern "C" fn collect_stats(ctx: *mut c_void, v: i32) -> i32 {
    0
}

fn stats_of(xs: &[i32]) -> Stats {
    Stats { count: 0, sum: 0, max: None }
}

extern "C" fn search_negative(ctx: *mut c_void, v: i32) -> i32 {
    0
}

fn first_negative(xs: &[i32]) -> Option<usize> {
    None
}

// Try it with Run.
fn main() {
    println!("{:?}", stats_of(&[3, -1, 4]));
    println!("{:?}", first_negative(&[3, -1, 4]));
}
