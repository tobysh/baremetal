Start with the two `impl Animal for ...` blocks; each needs `name` and `sound` returning `String`, for example `String::from("woof")`.
===
`speak_generic` and `speak_dyn` have the same body: `format!("{} says {}", a.name(), a.sound())`. Only the parameter type differs.
===
For `chorus`, `animals.iter()` yields `&Box<dyn Animal>`, and you can call methods on it directly. Reuse `speak_dyn(a.as_ref())`, or format again, then collect into a `Vec<String>` and call `.join("; ")`.
