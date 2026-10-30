use std::mem::{align_of, offset_of, size_of};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Wide {
    pub a: u8,
    pub b: u32,
    pub c: u8,
    pub d: u16,
}

// TODO: same four fields, reordered so the size is 8 bytes.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Tight {
    pub a: u8,
    pub b: u32,
    pub c: u8,
    pub d: u16,
}

#[derive(Debug, PartialEq)]
pub struct Report {
    pub size: usize,
    pub align: usize,
    pub off_b: usize,
    pub off_c: usize,
    pub off_d: usize,
}

fn wide_report() -> Report {
    Report { size: 0, align: 0, off_b: 0, off_c: 0, off_d: 0 }
}

fn tight_report() -> Report {
    Report { size: 0, align: 0, off_b: 0, off_c: 0, off_d: 0 }
}

fn widen(t: &Tight) -> Wide {
    Wide { a: 0, b: 0, c: 0, d: 0 }
}

fn to_bytes(w: &Wide) -> [u8; 12] {
    [0; 12]
}

// Try it with Run.
fn main() {
    println!("{:?}", wide_report());
    println!("{:?}", tight_report());
    println!("{:?}", to_bytes(&Wide { a: 1, b: 2, c: 3, d: 4 }));
}
