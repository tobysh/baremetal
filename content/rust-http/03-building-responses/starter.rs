fn reason_phrase(status: u16) -> &'static str {
    // Replace this with a match on the status code.
    "Unknown"
}

fn build_response(status: u16, content_type: &str, body: &str) -> String {
    // Replace this with a format! of the full response.
    String::new()
}

fn main() {
    let r = build_response(200, "text/plain", "héllo");
    println!("{:?}", r);
}
