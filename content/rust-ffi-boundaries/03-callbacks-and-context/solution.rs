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

unsafe fn for_each(xs: *const i32, len: usize, cb: Visit, ctx: *mut c_void) -> isize {
    if len == 0 {
        return 0;
    }
    if xs.is_null() {
        return -1;
    }
    for i in 0..len {
        let v = *xs.add(i);
        if cb(ctx, v) != 0 {
            return (i + 1) as isize;
        }
    }
    len as isize
}

extern "C" fn collect_stats(ctx: *mut c_void, v: i32) -> i32 {
    let s = unsafe { &mut *(ctx as *mut Stats) };
    s.count += 1;
    s.sum += v as i64;
    s.max = Some(s.max.map_or(v, |m| m.max(v)));
    0
}

fn stats_of(xs: &[i32]) -> Stats {
    let mut s = Stats { count: 0, sum: 0, max: None };
    unsafe { for_each(xs.as_ptr(), xs.len(), collect_stats, &mut s as *mut Stats as *mut c_void) };
    s
}

extern "C" fn search_negative(ctx: *mut c_void, v: i32) -> i32 {
    let s = unsafe { &mut *(ctx as *mut Search) };
    if v < 0 {
        s.found = Some(s.seen);
        1
    } else {
        s.seen += 1;
        0
    }
}

fn first_negative(xs: &[i32]) -> Option<usize> {
    let mut s = Search { seen: 0, found: None };
    unsafe { for_each(xs.as_ptr(), xs.len(), search_negative, &mut s as *mut Search as *mut c_void) };
    s.found
}

fn main() {
    println!("{:?}", stats_of(&[3, -1, 4]));
    println!("{:?}", first_negative(&[3, -1, 4]));
}
