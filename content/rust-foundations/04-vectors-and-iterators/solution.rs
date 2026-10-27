fn evens_squared(v: &[i32]) -> Vec<i32> {
    v.iter().filter(|&&x| x % 2 == 0).map(|&x| x * x).collect()
}

fn average(v: &[f64]) -> Option<f64> {
    if v.is_empty() {
        None
    } else {
        Some(v.iter().sum::<f64>() / v.len() as f64)
    }
}

fn main() {
    println!("{:?}", evens_squared(&[1, 2, 3, 4]));
    println!("{:?}", average(&[1.0, 2.0, 6.0]));
}
