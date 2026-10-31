unsafe fn sum_raw(p: *const i32, len: usize) -> i64 {
    0
}

unsafe fn fill_raw(p: *mut i32, len: usize, v: i32) {}

unsafe fn reverse_raw(p: *mut i32, len: usize) {}

unsafe fn copy_raw(dst: *mut i32, src: *const i32, len: usize) {}

// Try it with Run.
fn main() {
    let mut v = vec![1, 2, 3, 4, 5];
    unsafe {
        println!("{}", sum_raw(v.as_ptr(), v.len()));
        reverse_raw(v.as_mut_ptr(), v.len());
    }
    println!("{:?}", v);
}
