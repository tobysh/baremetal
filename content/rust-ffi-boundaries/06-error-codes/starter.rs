pub const OK: i32 = 0;
pub const NULL_ARG: i32 = -1;
pub const BAD_INPUT: i32 = -2;
pub const OVERFLOW: i32 = -3;
pub const PANICKED: i32 = -99;

unsafe fn checked_div(a: i32, b: i32, out: *mut i32) -> i32 {
    OK
}

unsafe fn parse_u8(s: *const u8, len: usize, out: *mut u8) -> i32 {
    OK
}

unsafe fn guarded(f: fn(i32) -> i32, arg: i32, out: *mut i32) -> i32 {
    OK
}

fn status_message(code: i32) -> &'static str {
    ""
}

fn into_result(code: i32) -> Result<(), i32> {
    Ok(())
}

// Try it with Run.
fn main() {
    let mut q = 0;
    let code = unsafe { checked_div(7, 2, &mut q) };
    println!("{} {}", status_message(code), q);
}
