#[derive(Debug, PartialEq)]
struct RequestLine {
    method: String,
    path: String,
    version: String,
}

fn parse_request_line(line: &str) -> Option<RequestLine> {
    // Replace this with real parsing.
    None
}

fn main() {
    println!("{:?}", parse_request_line("GET /index.html HTTP/1.1"));
    println!("{:?}", parse_request_line("nonsense"));
}
