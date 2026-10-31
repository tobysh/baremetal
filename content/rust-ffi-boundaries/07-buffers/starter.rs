unsafe fn write_decimal(buf: *mut u8, cap: usize, n: i64) -> isize {
    0
}

unsafe fn dup_bytes(src: *const u8, len: usize) -> *mut u8 {
    std::ptr::null_mut()
}

unsafe fn free_bytes(p: *mut u8, len: usize) {}

// Try it with Run.
fn main() {
    let mut buf = [0xAAu8; 8];
    let need = unsafe { write_decimal(buf.as_mut_ptr(), buf.len(), -12345) };
    println!("need {need}: {:?}", buf);
}
