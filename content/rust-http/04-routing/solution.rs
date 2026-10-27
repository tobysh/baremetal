fn route(method: &str, path: &str) -> (u16, String) {
    let segs: Vec<&str> = path.split('/').collect();
    let known = match segs.as_slice() {
        ["", ""] | ["", "health"] => true,
        ["", "echo", text] => !text.is_empty(),
        ["", "add", _, _] => true,
        _ => false,
    };
    if !known {
        return (404, "not found".to_string());
    }
    if method != "GET" {
        return (405, "method not allowed".to_string());
    }
    match segs.as_slice() {
        ["", ""] => (200, "Welcome to Baremetal".to_string()),
        ["", "health"] => (200, "ok".to_string()),
        ["", "echo", text] => (200, text.to_string()),
        ["", "add", a, b] => match (a.parse::<i64>(), b.parse::<i64>()) {
            (Ok(x), Ok(y)) => match x.checked_add(y) {
                Some(sum) => (200, sum.to_string()),
                None => (400, "bad number".to_string()),
            },
            _ => (400, "bad number".to_string()),
        },
        _ => (404, "not found".to_string()),
    }
}

fn main() {
    for (m, p) in [("GET", "/"), ("GET", "/echo/hi"), ("GET", "/add/2/3"), ("POST", "/health")] {
        println!("{} {} -> {:?}", m, p, route(m, p));
    }
}
