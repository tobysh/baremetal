trait Shape {
    fn name(&self) -> String;
    fn area(&self) -> u32;
    fn perimeter(&self) -> u32;

    fn describe(&self) -> String {
        format!("{}: area {}, perimeter {}", self.name(), self.area(), self.perimeter())
    }
}

struct Square {
    side: u32,
}

struct Rectangle {
    width: u32,
    height: u32,
}

// TODO: impl Shape for Square and for Rectangle.
impl Shape for Square {
    fn name(&self) -> String {
        String::new()
    }
    fn area(&self) -> u32 {
        0
    }
    fn perimeter(&self) -> u32 {
        0
    }
}

impl Shape for Rectangle {
    fn name(&self) -> String {
        String::new()
    }
    fn area(&self) -> u32 {
        0
    }
    fn perimeter(&self) -> u32 {
        0
    }
}

fn total_area<T: Shape>(shapes: &[T]) -> u32 {
    0
}

fn main() {
    let s = Square { side: 3 };
    println!("{}", s.describe());
}
