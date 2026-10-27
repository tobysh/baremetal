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

fn main() {
    let raw = "POST /x HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 2\r\n\r\nhi";
    println!("{:?}", parse_request(raw));
}
