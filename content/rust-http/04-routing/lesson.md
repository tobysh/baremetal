---
title: Routing
summary: Map method and path to a handler with match on slices of path segments, match guards and a small set of routes, including 404 and 405 errors.
minutes: 25
difficulty: hard
id: rust-http/routing
mode: harness
---
A web server does different things for different URLs. Choosing what to do from the **method** and **path** is called **routing**. Our router is a pure function: text in, `(status, body)` out. No sockets involved, which makes it very easy to test.

```rust
fn route(method: &str, path: &str) -> (u16, String)
```

## Splitting the path into segments

A path is a list of **segments** separated by `/`:

```rust
let segs: Vec<&str> = "/echo/hi".split('/').collect();
// ["", "echo", "hi"]
```

Notice the leading empty string: the path starts with `/`, so there is an empty piece before it. The root path `"/"` becomes `["", ""]`, and `"/echo/"` becomes `["", "echo", ""]`. Printing the vector while you experiment will save you a lot of confusion.

## Matching slices

Rust can `match` on a slice, destructuring it by shape:

```rust
match segs.as_slice() {
    ["", ""] => println!("root"),
    ["", "health"] => println!("health check"),
    ["", "echo", text] => println!("echo {}", text),
    ["", "add", a, b] => println!("add {} {}", a, b),
    _ => println!("something else"),
}
```

Each pattern states the exact length and the literal values it needs. `text`, `a` and `b` bind to whatever sits in that position. `as_slice()` turns the `Vec` into a `&[&str]` so patterns can see inside it.

## Match guards

A pattern cannot say "non-empty", but a **match guard** can add a condition with `if`:

```rust
["", "echo", text] if !text.is_empty() => { /* ... */ }
```

If the guard is false the arm is skipped and matching continues downward. That is how `/echo/` (empty text) falls through to the 404 arm.

## Parsing the numbers

`"12".parse::<i64>()` returns a `Result`. Matching on a tuple of two results handles both at once:

```rust
match (a.parse::<i64>(), b.parse::<i64>()) {
    (Ok(x), Ok(y)) => { /* sum */ }
    _ => { /* bad number */ }
}
```

## 404 versus 405

Two different errors, and the common mistake is mixing them up:

- **404 Not Found**: no such path exists at all.
- **405 Method Not Allowed**: the path exists, but not for this method (for example `POST /health`).

So you need to decide "does this path exist?" first, and only then check the method. A `POST` to a path that does not exist is a 404, not a 405.

## Your turn

Implement `route` following the table in the task. A clean approach is one `match` to decide whether the path is known, then a second to produce the result.
