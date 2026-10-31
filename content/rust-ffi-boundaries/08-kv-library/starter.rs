use std::cell::Cell;
use std::collections::BTreeMap;
use std::ffi::c_void;

thread_local! {
    pub static LIVE: Cell<isize> = Cell::new(0);
}

pub const OK: i32 = 0;
pub const NULL_ARG: i32 = -1;
pub const BAD_KEY: i32 = -2;
pub const OVERFLOW: i32 = -3;
pub const NOT_FOUND: i32 = -4;

pub struct Store {
    map: BTreeMap<Vec<u8>, i64>,
}

impl Store {
    fn new() -> Store {
        LIVE.with(|l| l.set(l.get() + 1));
        Store { map: BTreeMap::new() }
    }
}

impl Drop for Store {
    fn drop(&mut self) {
        LIVE.with(|l| l.set(l.get() - 1));
    }
}

#[repr(C)]
pub struct Entry {
    pub key: *const u8,
    pub key_len: usize,
    pub value: i64,
}

pub type EntryFn = extern "C" fn(ctx: *mut c_void, e: *const Entry) -> i32;

fn kv_new() -> *mut Store {
    std::ptr::null_mut()
}

unsafe fn kv_free(s: *mut Store) {}

unsafe fn kv_add(s: *mut Store, key: *const u8, key_len: usize, delta: i64, out: *mut i64) -> i32 {
    OK
}

unsafe fn kv_get(s: *const Store, key: *const u8, key_len: usize, out: *mut i64) -> i32 {
    OK
}

unsafe fn kv_remove(s: *mut Store, key: *const u8, key_len: usize) -> i32 {
    OK
}

unsafe fn kv_each(s: *const Store, cb: EntryFn, ctx: *mut c_void) -> isize {
    0
}

// Try it with Run.
fn main() {
    let s = kv_new();
    let mut v = 0;
    unsafe {
        kv_add(s, b"apples".as_ptr(), 6, 3, &mut v);
        println!("{v}");
        kv_free(s);
    }
}
