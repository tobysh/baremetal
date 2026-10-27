trait Animal {
    fn name(&self) -> String;
    fn sound(&self) -> String;
}

struct Dog;
struct Cat;

impl Animal for Dog {
    fn name(&self) -> String {
        String::from("Dog")
    }
    fn sound(&self) -> String {
        String::from("woof")
    }
}

impl Animal for Cat {
    fn name(&self) -> String {
        String::from("Cat")
    }
    fn sound(&self) -> String {
        String::from("meow")
    }
}

fn speak_generic<T: Animal>(a: &T) -> String {
    format!("{} says {}", a.name(), a.sound())
}

fn speak_dyn(a: &dyn Animal) -> String {
    format!("{} says {}", a.name(), a.sound())
}

fn chorus(animals: &[Box<dyn Animal>]) -> String {
    let parts: Vec<String> = animals.iter().map(|a| speak_dyn(a.as_ref())).collect();
    parts.join("; ")
}

fn main() {
    let zoo: Vec<Box<dyn Animal>> = vec![Box::new(Dog), Box::new(Cat)];
    println!("{}", chorus(&zoo));
}
