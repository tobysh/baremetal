#[test]
fn new_inventory_is_empty() {
    let inv = Inventory::new();
    assert_eq!(inv.total(), 0, "a new inventory should have total 0");
    assert!(inv.in_stock().is_empty(), "a new inventory should have nothing in stock");
    assert_eq!(inv.count("bolt"), 0, "count of an unknown item should be 0");
}

#[test]
fn add_creates_and_counts() {
    let mut inv = Inventory::new();
    inv.add("bolt", 10);
    assert_eq!(inv.count("bolt"), 10, "count(\"bolt\") should be 10 after add(\"bolt\", 10)");
}

#[test]
fn add_to_an_existing_item_accumulates() {
    let mut inv = Inventory::new();
    inv.add("bolt", 10);
    inv.add("bolt", 5);
    assert_eq!(inv.count("bolt"), 15, "adding twice should accumulate, got {}", inv.count("bolt"));
    assert_eq!(inv.items.len(), 1, "adding the same name twice should not create a second item");
}

#[test]
fn names_are_compared_exactly() {
    let mut inv = Inventory::new();
    inv.add("bolt", 1);
    inv.add("Bolt", 2);
    assert_eq!(inv.count("bolt"), 1, "\"bolt\" and \"Bolt\" are different items");
    assert_eq!(inv.count("Bolt"), 2, "\"bolt\" and \"Bolt\" are different items");
}

#[test]
fn total_sums_everything() {
    let mut inv = Inventory::new();
    inv.add("bolt", 10);
    inv.add("nut", 4);
    inv.add("washer", 100);
    assert_eq!(inv.total(), 114, "total should be 114, got {}", inv.total());
}

#[test]
fn remove_takes_stock_away() {
    let mut inv = Inventory::new();
    inv.add("bolt", 10);
    assert!(inv.remove("bolt", 3), "removing 3 of 10 should succeed");
    assert_eq!(inv.count("bolt"), 7, "7 should be left, got {}", inv.count("bolt"));
}

#[test]
fn remove_everything_is_allowed() {
    let mut inv = Inventory::new();
    inv.add("bolt", 4);
    assert!(inv.remove("bolt", 4), "removing exactly what is in stock should succeed");
    assert_eq!(inv.count("bolt"), 0, "nothing should be left");
}

#[test]
fn remove_too_many_changes_nothing() {
    let mut inv = Inventory::new();
    inv.add("bolt", 4);
    assert!(!inv.remove("bolt", 5), "removing more than is in stock should return false");
    assert_eq!(inv.count("bolt"), 4, "a failed remove must not change the quantity");
}

#[test]
fn remove_unknown_item_is_false() {
    let mut inv = Inventory::new();
    inv.add("bolt", 4);
    assert!(!inv.remove("nut", 1), "removing an unknown item should return false");
    assert!(!inv.remove("nut", 0), "removing 0 of an unknown item should still return false");
    assert_eq!(inv.items.len(), 1, "a failed remove must not create an item");
}

#[test]
fn in_stock_skips_empty_items_and_keeps_order() {
    let mut inv = Inventory::new();
    inv.add("bolt", 2);
    inv.add("nut", 1);
    inv.add("washer", 3);
    inv.remove("nut", 1);
    assert_eq!(inv.in_stock(), vec!["bolt".to_string(), "washer".to_string()], "in_stock should skip \"nut\" and keep insertion order");
}

#[test]
fn an_emptied_item_can_be_restocked() {
    let mut inv = Inventory::new();
    inv.add("bolt", 2);
    inv.add("nut", 1);
    inv.remove("bolt", 2);
    inv.add("bolt", 6);
    assert_eq!(inv.count("bolt"), 6, "restocking should work, got {}", inv.count("bolt"));
    assert_eq!(inv.in_stock(), vec!["bolt".to_string(), "nut".to_string()], "bolt keeps its original position");
}
