// Try it with Run.
fn main() {
    let program = "1 + 2\n\n10 / 0\n3 *\n7 < 9";
    for line in calc_lines(program) {
        println!("{}", line);
    }
    println!("total: {:?}", calc_total(program));
}
