fn close(a: f64, b: f64) -> bool {
    (a - b).abs() < 1e-6
}

#[test]
fn freezing_point_is_32_fahrenheit() {
    let r = celsius_to_fahrenheit(0.0);
    assert!(close(r, 32.0), "celsius_to_fahrenheit(0.0) should be 32.0, got {}", r);
}

#[test]
fn boiling_point_is_212_fahrenheit() {
    let r = celsius_to_fahrenheit(100.0);
    assert!(close(r, 212.0), "celsius_to_fahrenheit(100.0) should be 212.0, got {}", r);
}

#[test]
fn handles_negative_celsius() {
    let r = celsius_to_fahrenheit(-40.0);
    assert!(close(r, -40.0), "celsius_to_fahrenheit(-40.0) should be -40.0, got {}", r);
}

#[test]
fn handles_fractional_celsius() {
    let r = celsius_to_fahrenheit(36.6);
    assert!(close(r, 97.88), "celsius_to_fahrenheit(36.6) should be 97.88, got {}", r);
}

#[test]
fn year_divisible_by_four_is_leap() {
    assert!(is_leap_year(2024), "2024 is a leap year");
    assert!(is_leap_year(1996), "1996 is a leap year");
}

#[test]
fn ordinary_year_is_not_leap() {
    assert!(!is_leap_year(2023), "2023 is not a leap year");
    assert!(!is_leap_year(2001), "2001 is not a leap year");
}

#[test]
fn century_year_is_not_leap() {
    assert!(!is_leap_year(1900), "1900 is not a leap year");
    assert!(!is_leap_year(2100), "2100 is not a leap year");
}

#[test]
fn year_divisible_by_400_is_leap() {
    assert!(is_leap_year(2000), "2000 is a leap year");
    assert!(is_leap_year(1600), "1600 is a leap year");
}
