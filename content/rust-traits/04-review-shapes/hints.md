Start with the three `impl Shape` blocks. A `Square(f64)` is a tuple struct, so its side is `self.0`; the area is `self.0 * self.0`. A `Rect` is `self.w * self.h`, and a `Circle` is `PI * self.r * self.r`. Return `"square".to_string()` and so on from `name`. Leave `summary` alone: you inherit it.
===
`total_area` is `items.iter().map(|s| s.area()).sum()`. `report` maps each item to its `summary()`, collects into a `Vec<String>`, and calls `.join("; ")`. Because `items` holds `Box<dyn Shape>`, calling `s.summary()` works straight through the box.
===
For `describe_largest`, `max()` will not compile because `f64` has no total order. Use `items.iter().max_by(|a, b| a.area().partial_cmp(&b.area()).unwrap())`, which gives an `Option<&Box<dyn Shape>>`, and then `.map(|s| s.summary())`. An empty slice gives `None` for free.
