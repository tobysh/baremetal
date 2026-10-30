use std::mem::{align_of, offset_of, size_of};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Wide {
    pub a: u8,
    pub b: u32,
    pub c: u8,
    pub d: u16,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Tight {
    pub b: u32,
    pub d: u16,
    pub a: u8,
    pub c: u8,
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
    Report {
        size: size_of::<Wide>(),
        align: align_of::<Wide>(),
        off_b: offset_of!(Wide, b),
        off_c: offset_of!(Wide, c),
        off_d: offset_of!(Wide, d),
    }
}

fn tight_report() -> Report {
    Report {
        size: size_of::<Tight>(),
        align: align_of::<Tight>(),
        off_b: offset_of!(Tight, b),
        off_c: offset_of!(Tight, c),
        off_d: offset_of!(Tight, d),
    }
}

fn widen(t: &Tight) -> Wide {
    Wide { a: t.a, b: t.b, c: t.c, d: t.d }
}

fn to_bytes(w: &Wide) -> [u8; 12] {
    let mut out = [0u8; 12];
    out[offset_of!(Wide, a)] = w.a;
    let o = offset_of!(Wide, b);
    out[o..o + 4].copy_from_slice(&w.b.to_le_bytes());
    out[offset_of!(Wide, c)] = w.c;
    let o = offset_of!(Wide, d);
    out[o..o + 2].copy_from_slice(&w.d.to_le_bytes());
    out
}

fn main() {
    println!("{:?}", wide_report());
    println!("{:?}", tight_report());
    println!("{:?}", to_bytes(&Wide { a: 1, b: 2, c: 3, d: 4 }));
}
