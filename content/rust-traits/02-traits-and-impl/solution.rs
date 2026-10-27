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

impl Shape for Square {
    fn name(&self) -> String {
        String::from("square")
    }
    fn area(&self) -> u32 {
        self.side * self.side
    }
    fn perimeter(&self) -> u32 {
        4 * self.side
    }
}

impl Shape for Rectangle {
    fn name(&self) -> String {
        String::from("rectangle")
    }
    fn area(&self) -> u32 {
        self.width * self.height
    }
    fn perimeter(&self) -> u32 {
        2 * (self.width + self.height)
    }
}

fn total_area<T: Shape>(shapes: &[T]) -> u32 {
    shapes.iter().map(|s| s.area()).sum()
}

fn main() {
    let s = Square { side: 3 };
    println!("{}", s.describe());
}
