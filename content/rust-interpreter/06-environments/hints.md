Construct a frame with `Rc::new(RefCell::new(Frame { ... }))`. A child uses `Some(self.clone())` for its parent and clones `self.out`.
===
Look in the current map before recursing. `assign` must return immediately after changing a local slot; only an absent local name should reach the parent.
