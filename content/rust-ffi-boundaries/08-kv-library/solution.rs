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

unsafe fn key<'a>(s: *const Store, k: *const u8, n: usize) -> Result<&'a [u8], i32> {
    if s.is_null() {
        return Err(NULL_ARG);
    }
    if n == 0 {
        return Err(BAD_KEY);
    }
    if k.is_null() {
        return Err(NULL_ARG);
    }
    Ok(std::slice::from_raw_parts(k, n))
}

fn kv_new() -> *mut Store {
    Box::into_raw(Box::new(Store::new()))
}

unsafe fn kv_free(s: *mut Store) {
    if !s.is_null() {
        drop(Box::from_raw(s));
    }
}

unsafe fn kv_add(s: *mut Store, key_p: *const u8, key_len: usize, delta: i64, out: *mut i64) -> i32 {
    let k = match key(s, key_p, key_len) {
        Ok(k) => k,
        Err(c) => return c,
    };
    let store = &mut *s;
    let cur = store.map.get(k).copied().unwrap_or(0);
    match cur.checked_add(delta) {
        None => OVERFLOW,
        Some(v) => {
            store.map.insert(k.to_vec(), v);
            if !out.is_null() {
                *out = v;
            }
            OK
        }
    }
}

unsafe fn kv_get(s: *const Store, key_p: *const u8, key_len: usize, out: *mut i64) -> i32 {
    let k = match key(s, key_p, key_len) {
        Ok(k) => k,
        Err(c) => return c,
    };
    match (*s).map.get(k) {
        None => NOT_FOUND,
        Some(&v) => {
            if !out.is_null() {
                *out = v;
            }
            OK
        }
    }
}

unsafe fn kv_remove(s: *mut Store, key_p: *const u8, key_len: usize) -> i32 {
    let k = match key(s, key_p, key_len) {
        Ok(k) => k,
        Err(c) => return c,
    };
    if (*s).map.remove(k).is_some() { OK } else { NOT_FOUND }
}

unsafe fn kv_each(s: *const Store, cb: EntryFn, ctx: *mut c_void) -> isize {
    if s.is_null() {
        return -1;
    }
    let mut n = 0;
    for (k, &v) in &(*s).map {
        let e = Entry { key: k.as_ptr(), key_len: k.len(), value: v };
        n += 1;
        if cb(ctx, &e) != 0 {
            break;
        }
    }
    n
}

fn main() {
    let s = kv_new();
    let mut v = 0;
    unsafe {
        kv_add(s, b"apples".as_ptr(), 6, 3, &mut v);
        println!("{v}");
        kv_free(s);
    }
}
