use std::collections::HashMap;

// ---- from the previous lesson ----
#[derive(Debug, PartialEq)]
struct RequestLine {
    method: String,
    path: String,
    version: String,
}

fn parse_request_line(line: &str) -> Option<RequestLine> {
    let parts: Vec<&str> = line.split(' ').collect();
    if parts.len() != 3 || !parts[2].starts_with("HTTP/") {
        return None;
    }
    Some(RequestLine {
        method: parts[0].to_string(),
        path: parts[1].to_string(),
        version: parts[2].to_string(),
    })
}
// ---- end of previous lesson ----

#[derive(Debug)]
struct Request {
    method: String,
    path: String,
    headers: HashMap<String, String>,
    body: String,
}

fn parse_request(raw: &str) -> Result<Request, String> {
    // Replace this with real parsing.
    Err("not implemented".to_string())
}

fn main() {
    let raw = "POST /x HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 2\r\n\r\nhi";
    println!("{:?}", parse_request(raw));
}
