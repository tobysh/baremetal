fn route(method: &str, path: &str) -> (u16, String) {
    // Replace this with real routing.
    (404, "not found".to_string())
}

fn main() {
    for (m, p) in [("GET", "/"), ("GET", "/echo/hi"), ("GET", "/add/2/3"), ("POST", "/health")] {
        println!("{} {} -> {:?}", m, p, route(m, p));
    }
}
