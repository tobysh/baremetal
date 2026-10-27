struct Item {
    name: String,
    qty: u32,
}

struct Inventory {
    items: Vec<Item>,
}

impl Inventory {
    fn new() -> Inventory {
        Inventory { items: Vec::new() }
    }

    fn add(&mut self, name: &str, qty: u32) {}

    fn remove(&mut self, name: &str, qty: u32) -> bool {
        false
    }

    fn count(&self, name: &str) -> u32 {
        0
    }

    fn total(&self) -> u32 {
        0
    }

    fn in_stock(&self) -> Vec<String> {
        Vec::new()
    }
}

// Try your inventory with Run.
fn main() {
    let mut inv = Inventory::new();
    inv.add("bolt", 10);
    inv.add("nut", 4);
    inv.remove("bolt", 3);
    println!("bolts: {}, total: {}", inv.count("bolt"), inv.total());
    println!("in stock: {:?}", inv.in_stock());
}
