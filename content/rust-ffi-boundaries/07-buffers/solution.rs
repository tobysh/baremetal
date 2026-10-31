unsafe fn write_decimal(buf: *mut u8, cap: usize, n: i64) -> isize {
    let s = n.to_string();
    let need = s.len();
    if cap == 0 {
        return need as isize;
    }
    if buf.is_null() {
        return -1;
    }
    let k = need.min(cap - 1);
    std::ptr::copy_nonoverlapping(s.as_ptr(), buf, k);
    *buf.add(k) = 0;
    need as isize
}

unsafe fn dup_bytes(src: *const u8, len: usize) -> *mut u8 {
    if len == 0 || src.is_null() {
        return std::ptr::null_mut();
    }
    let b: Box<[u8]> = std::slice::from_raw_parts(src, len).to_vec().into_boxed_slice();
    Box::into_raw(b) as *mut u8
}

unsafe fn free_bytes(p: *mut u8, len: usize) {
    if !p.is_null() {
        drop(Box::from_raw(std::ptr::slice_from_raw_parts_mut(p, len)));
    }
}

fn main() {
    let mut buf = [0xAAu8; 8];
    let need = unsafe { write_decimal(buf.as_mut_ptr(), buf.len(), -12345) };
    println!("need {need}: {:?}", buf);
}
