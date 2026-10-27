use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;
use std::time::Duration;

// ---- from previous lessons ----
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

fn handle_connection(mut stream: TcpStream) {
    let _ = stream.set_read_timeout(Some(Duration::from_secs(5)));
    let mut buf: Vec<u8> = Vec::new();
    let mut chunk = [0u8; 1024];
    loop {
        match stream.read(&mut chunk) {
            Ok(0) => break,
            Ok(n) => buf.extend_from_slice(&chunk[..n]),
            Err(_) => break,
        }
        if buf.windows(4).any(|w| w == b"\r\n\r\n") || buf.len() >= 8192 {
            break;
        }
    }
    let text = String::from_utf8_lossy(&buf);
    let first = text.lines().next().unwrap_or("");
    let response = match parse_request_line(first) {
        Some(rl) => {
            let (status, body) = route(&rl.method, &rl.path);
            build_response(status, "text/plain", &body)
        }
        None => build_response(400, "text/plain", "bad request"),
    };
    let _ = stream.write_all(response.as_bytes());
    let _ = stream.flush();
}
// ---- end of previous lessons ----

// Sequential: one slow client blocks everyone. Make it concurrent.
fn serve(listener: TcpListener, max_connections: usize) {
    for _ in 0..max_connections {
        if let Ok((stream, _)) = listener.accept() {
            handle_connection(stream);
        }
    }
}

fn main() {
    let listener = TcpListener::bind("127.0.0.1:8080").unwrap();
    println!("listening on http://127.0.0.1:8080");
    serve(listener, 100);
}
