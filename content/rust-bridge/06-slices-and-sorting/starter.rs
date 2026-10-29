fn median(v: &[i32]) -> Option<f64> {
    None
}

fn top_n(v: &[i32], n: usize) -> Vec<i32> {
    Vec::new()
}

fn insert_sorted(v: &mut Vec<i32>, x: i32) {
}

fn unique_sorted(v: &[i32]) -> Vec<i32> {
    Vec::new()
}

fn ends(v: &[i32]) -> Option<(i32, i32)> {
    None
}

fn sort_people(people: &mut Vec<(String, u32)>) {
}

// Try it with Run.
fn main() {
    println!("{:?}", median(&[5, 1, 3, 2]));
    println!("{:?}", top_n(&[4, 9, 1, 9, 3], 3));
    let mut v = vec![1, 3, 5];
    insert_sorted(&mut v, 4);
    println!("{:?}", v);
    println!("{:?}", unique_sorted(&[3, 1, 3, 2, 1]));
    println!("{:?}", ends(&[7, 8, 9]));
    let mut p = vec![("Zed".to_string(), 30), ("Amy".to_string(), 30), ("Bo".to_string(), 41)];
    sort_people(&mut p);
    println!("{:?}", p);
}
