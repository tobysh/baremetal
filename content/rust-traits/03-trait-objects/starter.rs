trait Animal {
    fn name(&self) -> String;
    fn sound(&self) -> String;
}

struct Dog;
struct Cat;

impl Animal for Dog {
    fn name(&self) -> String {
        String::new()
    }
    fn sound(&self) -> String {
        String::new()
    }
}

impl Animal for Cat {
    fn name(&self) -> String {
        String::new()
    }
    fn sound(&self) -> String {
        String::new()
    }
}

fn speak_generic<T: Animal>(a: &T) -> String {
    String::new()
}

fn speak_dyn(a: &dyn Animal) -> String {
    String::new()
}

fn chorus(animals: &[Box<dyn Animal>]) -> String {
    String::new()
}

fn main() {
    let zoo: Vec<Box<dyn Animal>> = vec![Box::new(Dog), Box::new(Cat)];
    println!("{}", chorus(&zoo));
}
