// ---- your code from the earlier lessons, ready to use ----
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
    let (head, body) = raw.split_once("\r\n\r\n").unwrap_or((raw, ""));
    if head.is_empty() {
        return Err("missing request line".to_string());
    }
    let mut lines = head.split("\r\n");
    let first = lines.next().unwrap_or("");
    let rl = parse_request_line(first).ok_or_else(|| format!("bad request line: {}", first))?;
    let mut headers = HashMap::new();
    for line in lines {
        let (name, value) = line
            .split_once(':')
            .ok_or_else(|| format!("malformed header: {}", line))?;
        headers.insert(name.trim().to_lowercase(), value.trim().to_string());
    }
    Ok(Request { method: rl.method, path: rl.path, headers, body: body.to_string() })
}

fn reason_phrase(status: u16) -> &'static str {
    match status {
        200 => "OK",
        201 => "Created",
        400 => "Bad Request",
        404 => "Not Found",
        405 => "Method Not Allowed",
        500 => "Internal Server Error",
        _ => "Unknown",
    }
}

fn build_response(status: u16, content_type: &str, body: &str) -> String {
    format!(
        "HTTP/1.1 {} {}\r\nContent-Type: {}\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}",
        status,
        reason_phrase(status),
        content_type,
        body.len(),
        body
    )
}

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

// ---- new in this lesson ----

fn body_matches_length(req: &Request) -> bool {
    true
}

fn handle(raw: &str) -> String {
    String::new()
}

// Try your handler with Run.
fn main() {
    let raw = "GET /echo/hi HTTP/1.1\r\nHost: x\r\n\r\n";
    println!("{:?}", handle(raw));
}
