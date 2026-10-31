unsafe fn sum_raw(p: *const i32, len: usize) -> i64 {
    if p.is_null() {
        return 0;
    }
    std::slice::from_raw_parts(p, len).iter().map(|&x| x as i64).sum()
}

unsafe fn fill_raw(p: *mut i32, len: usize, v: i32) {
    if p.is_null() {
        return;
    }
    std::slice::from_raw_parts_mut(p, len).fill(v);
}

unsafe fn reverse_raw(p: *mut i32, len: usize) {
    if p.is_null() {
        return;
    }
    std::slice::from_raw_parts_mut(p, len).reverse();
}

unsafe fn copy_raw(dst: *mut i32, src: *const i32, len: usize) {
    if dst.is_null() || src.is_null() {
        return;
    }
    std::ptr::copy(src, dst, len);
}

fn main() {
    let mut v = vec![1, 2, 3, 4, 5];
    unsafe {
        println!("{}", sum_raw(v.as_ptr(), v.len()));
        reverse_raw(v.as_mut_ptr(), v.len());
    }
    println!("{:?}", v);
}
