fn make_adder(n: i32) -> impl Fn(i32) -> i32 {
    move |x| x + n
}

fn apply_n<F: Fn(i32) -> i32>(f: F, x: i32, times: u32) -> i32 {
    let mut value = x;
    for _ in 0..times {
        value = f(value);
    }
    value
}

fn make_counter() -> impl FnMut() -> u32 {
    let mut count = 0;
    move || {
        count += 1;
        count
    }
}

fn main() {
    let add3 = make_adder(3);
    println!("{}", add3(4));
    println!("{}", apply_n(|x| x * 2, 1, 5));
    let mut c = make_counter();
    println!("{} {} {}", c(), c(), c());
}
