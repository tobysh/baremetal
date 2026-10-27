The starter defines `trait Animal { fn name(&self) -> String; fn sound(&self) -> String; }` and unit structs `Dog` and `Cat`.

- Implement `Animal` for `Dog` (name `"Dog"`, sound `"woof"`) and `Cat` (name `"Cat"`, sound `"meow"`).
- `fn speak_generic<T: Animal>(a: &T) -> String` returns `"<name> says <sound>"`, for example `"Dog says woof"`.
- `fn speak_dyn(a: &dyn Animal) -> String` returns the same text, using a trait object.
- `fn chorus(animals: &[Box<dyn Animal>]) -> String` returns every animal's `"<name> says <sound>"` joined with `"; "`, in order. An empty slice gives an empty string.
