fn celsius_to_fahrenheit(c: f64) -> f64 {
    c * 9.0 / 5.0 + 32.0
}

fn is_leap_year(year: u32) -> bool {
    year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)
}

fn main() {
    println!("100C = {}F", celsius_to_fahrenheit(100.0));
    println!("2024 leap? {}", is_leap_year(2024));
}
