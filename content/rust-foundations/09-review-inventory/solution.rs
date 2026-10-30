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

    fn add(&mut self, name: &str, qty: u32) {
        if let Some(item) = self.items.iter_mut().find(|i| i.name == name) {
            item.qty += qty;
        } else {
            self.items.push(Item { name: name.to_string(), qty });
        }
    }

    fn remove(&mut self, name: &str, qty: u32) -> bool {
        match self.items.iter_mut().find(|i| i.name == name) {
            Some(item) if item.qty >= qty => {
                item.qty -= qty;
                true
            }
            _ => false,
        }
    }

    fn count(&self, name: &str) -> u32 {
        self.items.iter().find(|i| i.name == name).map(|i| i.qty).unwrap_or(0)
    }

    fn total(&self) -> u32 {
        self.items.iter().map(|i| i.qty).sum()
    }

    fn in_stock(&self) -> Vec<String> {
        self.items.iter().filter(|i| i.qty > 0).map(|i| i.name.clone()).collect()
    }
}

fn main() {
    let mut inv = Inventory::new();
    inv.add("bolt", 10);
    inv.add("nut", 4);
    inv.remove("bolt", 3);
    println!("bolts: {}, total: {}", inv.count("bolt"), inv.total());
    println!("in stock: {:?}", inv.in_stock());
}
