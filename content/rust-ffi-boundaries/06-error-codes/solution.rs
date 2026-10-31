pub const OK: i32 = 0;
pub const NULL_ARG: i32 = -1;
pub const BAD_INPUT: i32 = -2;
pub const OVERFLOW: i32 = -3;
pub const PANICKED: i32 = -99;

unsafe fn checked_div(a: i32, b: i32, out: *mut i32) -> i32 {
    if out.is_null() {
        return NULL_ARG;
    }
    if b == 0 {
        return BAD_INPUT;
    }
    match a.checked_div(b) {
        None => OVERFLOW,
        Some(q) => {
            *out = q;
            OK
        }
    }
}

unsafe fn parse_u8(s: *const u8, len: usize, out: *mut u8) -> i32 {
    if out.is_null() || (s.is_null() && len > 0) {
        return NULL_ARG;
    }
    if len == 0 {
        return BAD_INPUT;
    }
    let bytes = std::slice::from_raw_parts(s, len);
    if !bytes.iter().all(|b| b.is_ascii_digit()) {
        return BAD_INPUT;
    }
    let mut acc: u32 = 0;
    for &b in bytes {
        acc = acc * 10 + (b - b'0') as u32;
        if acc > 255 {
            return OVERFLOW;
        }
    }
    *out = acc as u8;
    OK
}

unsafe fn guarded(f: fn(i32) -> i32, arg: i32, out: *mut i32) -> i32 {
    if out.is_null() {
        return NULL_ARG;
    }
    match std::panic::catch_unwind(|| f(arg)) {
        Ok(v) => {
            *out = v;
            OK
        }
        Err(_) => PANICKED,
    }
}

fn status_message(code: i32) -> &'static str {
    match code {
        OK => "ok",
        NULL_ARG => "null argument",
        BAD_INPUT => "bad input",
        OVERFLOW => "overflow",
        PANICKED => "panicked",
        _ => "unknown",
    }
}

fn into_result(code: i32) -> Result<(), i32> {
    if code == OK { Ok(()) } else { Err(code) }
}

fn main() {
    let mut q = 0;
    let code = unsafe { checked_div(7, 2, &mut q) };
    println!("{} {}", status_message(code), q);
}
