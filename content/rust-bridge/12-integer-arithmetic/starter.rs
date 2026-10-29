fn checked_factorial(n: u32) -> Option<u64> {
    None
}

fn fnv1a(data: &[u8]) -> u32 {
    0
}

fn clamp_to_i8(x: i32) -> i8 {
    0
}

fn total_saturating(v: &[u8]) -> u8 {
    0
}

fn average(a: i32, b: i32) -> i32 {
    0
}

fn bits_set(x: u32) -> u32 {
    0
}

fn set_bit(x: u32, i: u32, on: bool) -> u32 {
    x
}

// Try it with Run.
fn main() {
    println!("{:?} {:?}", checked_factorial(20), checked_factorial(21));
    println!("{:#x}", fnv1a(b"hello"));
    println!("{} {}", clamp_to_i8(300), clamp_to_i8(-300));
    println!("{}", total_saturating(&[200, 100]));
    println!("{}", average(i32::MAX, i32::MAX));
    println!("{}", bits_set(0b1011));
    println!("{:#b}", set_bit(0b1000, 1, true));
}
