#[test]
fn factorials() {
    assert_eq!(checked_factorial(0), Some(1), "0!");
    assert_eq!(checked_factorial(1), Some(1), "1!");
    assert_eq!(checked_factorial(5), Some(120), "5!");
    assert_eq!(checked_factorial(20), Some(2_432_902_008_176_640_000), "20! fits");
    assert_eq!(checked_factorial(21), None, "21! does not");
    assert_eq!(checked_factorial(1000), None, "far too big, and must not panic");
}

#[test]
fn hashing() {
    assert_eq!(fnv1a(b""), 2166136261, "empty input is the offset basis");
    assert_eq!(fnv1a(b"a"), 0xe40c292c, "known value for a");
    assert_eq!(fnv1a(b"foobar"), 0xbf9cf968, "known value for foobar");
    assert_ne!(fnv1a(b"ab"), fnv1a(b"ba"), "order matters");
}

#[test]
fn clamping() {
    assert_eq!(clamp_to_i8(0), 0, "zero");
    assert_eq!(clamp_to_i8(127), 127, "max");
    assert_eq!(clamp_to_i8(128), 127, "just over");
    assert_eq!(clamp_to_i8(300), 127, "far over (as i8 would give 44)");
    assert_eq!(clamp_to_i8(-128), -128, "min");
    assert_eq!(clamp_to_i8(-129), -128, "just under");
    assert_eq!(clamp_to_i8(i32::MIN), -128, "far under");
}

#[test]
fn saturating_totals() {
    assert_eq!(total_saturating(&[1, 2, 3]), 6, "small");
    assert_eq!(total_saturating(&[200, 100]), 255, "saturates");
    assert_eq!(total_saturating(&[255, 255, 255]), 255, "stays saturated");
    assert_eq!(total_saturating(&[250, 5]), 255, "exactly 255");
    assert_eq!(total_saturating(&[]), 0, "empty");
}

#[test]
fn averages() {
    assert_eq!(average(4, 6), 5, "even sum");
    assert_eq!(average(3, 4), 3, "rounds down");
    assert_eq!(average(-3, -4), -4, "rounds toward negative infinity");
    assert_eq!(average(i32::MAX, i32::MAX), i32::MAX, "no overflow");
    assert_eq!(average(i32::MIN, i32::MIN), i32::MIN, "no underflow");
    assert_eq!(average(-1, 0), -1, "negative and zero");
}

#[test]
fn counting_bits() {
    assert_eq!(bits_set(0), 0, "zero");
    assert_eq!(bits_set(0b1011), 3, "three");
    assert_eq!(bits_set(u32::MAX), 32, "all");
    assert_eq!(bits_set(0x8000_0000), 1, "top bit");
    assert_eq!(bits_set(0xF0F0), 8, "pattern");
}

#[test]
fn setting_bits() {
    assert_eq!(set_bit(0b1000, 1, true), 0b1010, "set");
    assert_eq!(set_bit(0b1010, 1, false), 0b1000, "clear");
    assert_eq!(set_bit(0b1010, 1, true), 0b1010, "already set");
    assert_eq!(set_bit(0, 31, true), 0x8000_0000, "top bit");
    assert_eq!(set_bit(u32::MAX, 0, false), u32::MAX - 1, "bottom bit");
    assert_eq!(set_bit(5, 32, true), 5, "out of range is ignored");
    assert_eq!(set_bit(5, 100, false), 5, "far out of range is ignored");
}
