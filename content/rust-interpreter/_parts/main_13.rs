// Try it with Run.
fn main() {
    println!("{:?}", eval_via_vm("(1 + 2) * 7 - 4"));
    let mut vm = Vm::new();
    vm.run(&[Op::Const(5), Op::Print]).unwrap();
    println!("{:?}", vm.out);
}
