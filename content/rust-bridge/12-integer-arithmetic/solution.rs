fn checked_factorial(n: u32) -> Option<u64> {
    let mut acc: u64 = 1;
    for k in 2..=n as u64 {
        acc = acc.checked_mul(k)?;
    }
    Some(acc)
}

fn fnv1a(data: &[u8]) -> u32 {
    data.iter()
        .fold(2166136261u32, |h, &b| (h ^ b as u32).wrapping_mul(16777619))
}

fn clamp_to_i8(x: i32) -> i8 {
    x.clamp(-128, 127) as i8
}

fn total_saturating(v: &[u8]) -> u8 {
    v.iter().fold(0u8, |acc, &b| acc.saturating_add(b))
}

fn average(a: i32, b: i32) -> i32 {
    (a as i64 + b as i64).div_euclid(2) as i32
}

fn bits_set(mut x: u32) -> u32 {
    let mut n = 0;
    while x != 0 {
        x &= x - 1;
        n += 1;
    }
    n
}

fn set_bit(x: u32, i: u32, on: bool) -> u32 {
    if i >= 32 {
        return x;
    }
    let m = 1u32 << i;
    if on { x | m } else { x & !m }
}

fn main() {
    println!("{:?}", checked_factorial(20));
}
