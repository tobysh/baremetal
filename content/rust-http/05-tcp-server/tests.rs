use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;
use std::time::Duration;

fn start_server() -> (std::net::SocketAddr, thread::JoinHandle<()>) {
    let listener = TcpListener::bind("127.0.0.1:0").expect("bind");
    let addr = listener.local_addr().unwrap();
    let h = thread::spawn(move || {
        let (stream, _) = listener.accept().expect("accept");
        handle_connection(stream);
    });
    (addr, h)
}

fn connect(addr: std::net::SocketAddr) -> TcpStream {
    let c = TcpStream::connect(addr).expect("connect");
    c.set_read_timeout(Some(Duration::from_secs(3))).unwrap();
    c
}

fn finish(mut c: TcpStream, h: thread::JoinHandle<()>) -> String {
    let mut resp = String::new();
    c.read_to_string(&mut resp)
        .expect("reading the response failed or timed out: did the server reply and close the connection?");
    h.join().expect("server thread panicked");
    resp
}

#[test]
fn get_health_returns_ok() {
    let (addr, h) = start_server();
    let mut c = connect(addr);
    c.write_all(b"GET /health HTTP/1.1\r\nHost: x\r\n\r\n").unwrap();
    let resp = finish(c, h);
    assert!(resp.starts_with("HTTP/1.1 200 OK\r\n"), "got: {:?}", resp);
    assert!(resp.ends_with("\r\n\r\nok"), "body should be 'ok', got: {:?}", resp);
    assert!(resp.contains("Content-Length: 2\r\n"), "got: {:?}", resp);
}

#[test]
fn routes_echo_and_add() {
    let (addr, h) = start_server();
    let mut c = connect(addr);
    c.write_all(b"GET /add/20/22 HTTP/1.1\r\n\r\n").unwrap();
    let resp = finish(c, h);
    assert!(resp.starts_with("HTTP/1.1 200 OK\r\n"), "got: {:?}", resp);
    assert!(resp.ends_with("\r\n\r\n42"), "got: {:?}", resp);
}

#[test]
fn unknown_path_gets_404() {
    let (addr, h) = start_server();
    let mut c = connect(addr);
    c.write_all(b"GET /nope HTTP/1.1\r\n\r\n").unwrap();
    let resp = finish(c, h);
    assert!(resp.starts_with("HTTP/1.1 404 Not Found\r\n"), "got: {:?}", resp);
    assert!(resp.ends_with("not found"), "got: {:?}", resp);
}

#[test]
fn post_to_known_path_gets_405() {
    let (addr, h) = start_server();
    let mut c = connect(addr);
    c.write_all(b"POST /health HTTP/1.1\r\n\r\n").unwrap();
    let resp = finish(c, h);
    assert!(resp.starts_with("HTTP/1.1 405 Method Not Allowed\r\n"), "got: {:?}", resp);
}

#[test]
fn request_arriving_in_two_writes_is_read_fully() {
    let (addr, h) = start_server();
    let mut c = connect(addr);
    c.set_nodelay(true).unwrap();
    c.write_all(b"GET /echo/pie").unwrap();
    c.flush().unwrap();
    thread::sleep(Duration::from_millis(300));
    c.write_all(b"ces HTTP/1.1\r\nHost: x\r\n\r\n").unwrap();
    let resp = finish(c, h);
    assert!(resp.starts_with("HTTP/1.1 200 OK\r\n"), "a single read() only saw half the request; got: {:?}", resp);
    assert!(resp.ends_with("\r\n\r\npieces"), "got: {:?}", resp);
}

#[test]
fn blank_line_split_across_writes() {
    let (addr, h) = start_server();
    let mut c = connect(addr);
    c.set_nodelay(true).unwrap();
    c.write_all(b"GET /health HTTP/1.1\r\n\r").unwrap();
    c.flush().unwrap();
    thread::sleep(Duration::from_millis(300));
    c.write_all(b"\n").unwrap();
    let resp = finish(c, h);
    assert!(resp.starts_with("HTTP/1.1 200 OK\r\n"), "got: {:?}", resp);
}

#[test]
fn malformed_request_gets_400() {
    let (addr, h) = start_server();
    let mut c = connect(addr);
    c.write_all(b"this is not http\r\n\r\n").unwrap();
    let resp = finish(c, h);
    assert!(resp.starts_with("HTTP/1.1 400 Bad Request\r\n"), "got: {:?}", resp);
    assert!(resp.ends_with("\r\n\r\nbad request"), "got: {:?}", resp);
}

#[test]
fn client_that_sends_nothing_and_closes_does_not_crash_the_server() {
    let (addr, h) = start_server();
    let c = connect(addr);
    c.shutdown(std::net::Shutdown::Write).unwrap();
    let mut c = c;
    let mut resp = String::new();
    let _ = c.read_to_string(&mut resp);
    h.join().expect("server thread panicked on empty input");
    assert!(resp.is_empty() || resp.starts_with("HTTP/1.1 400"), "got: {:?}", resp);
}
