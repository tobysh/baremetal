use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;
use std::time::{Duration, Instant};

fn client(addr: std::net::SocketAddr, timeout: Duration) -> TcpStream {
    let c = TcpStream::connect(addr).expect("connect");
    c.set_read_timeout(Some(timeout)).unwrap();
    c
}

#[test]
fn slow_client_does_not_block_a_fast_one() {
    let listener = TcpListener::bind("127.0.0.1:0").unwrap();
    let addr = listener.local_addr().unwrap();
    let server = thread::spawn(move || serve(listener, 2));
    let start = Instant::now();

    // Client A connects and says nothing for a while.
    let mut a = client(addr, Duration::from_secs(4));
    thread::sleep(Duration::from_millis(200));

    // Client B must still get an answer quickly.
    let mut b = client(addr, Duration::from_secs(1));
    b.write_all(b"GET /health HTTP/1.1\r\nHost: x\r\n\r\n").unwrap();
    let mut resp_b = String::new();
    b.read_to_string(&mut resp_b)
        .expect("client B got no reply within 1s while client A was idle: is the server handling connections concurrently?");
    assert!(resp_b.starts_with("HTTP/1.1 200 OK\r\n"), "B got: {:?}", resp_b);
    assert!(resp_b.ends_with("\r\n\r\nok"), "B got: {:?}", resp_b);

    // A finally speaks, and is served too.
    let wait = Duration::from_millis(1500);
    if start.elapsed() < wait {
        thread::sleep(wait - start.elapsed());
    }
    a.write_all(b"GET /echo/late HTTP/1.1\r\nHost: x\r\n\r\n").unwrap();
    let mut resp_a = String::new();
    a.read_to_string(&mut resp_a).expect("client A should still be answered");
    assert!(resp_a.starts_with("HTTP/1.1 200 OK\r\n"), "A got: {:?}", resp_a);
    assert!(resp_a.ends_with("\r\n\r\nlate"), "A got: {:?}", resp_a);

    server.join().expect("serve should return after max_connections");
}

#[test]
fn serves_several_clients_in_a_row() {
    let listener = TcpListener::bind("127.0.0.1:0").unwrap();
    let addr = listener.local_addr().unwrap();
    let server = thread::spawn(move || serve(listener, 3));
    for i in 0..3 {
        let mut c = client(addr, Duration::from_secs(3));
        c.write_all(format!("GET /add/{}/10 HTTP/1.1\r\n\r\n", i).as_bytes()).unwrap();
        let mut resp = String::new();
        c.read_to_string(&mut resp).expect("response");
        assert!(resp.ends_with(&format!("\r\n\r\n{}", i + 10)), "got: {:?}", resp);
    }
    server.join().expect("serve should return after 3 connections");
}

#[test]
fn many_clients_at_once() {
    let listener = TcpListener::bind("127.0.0.1:0").unwrap();
    let addr = listener.local_addr().unwrap();
    let server = thread::spawn(move || serve(listener, 5));
    let clients: Vec<_> = (0..5)
        .map(|i| {
            thread::spawn(move || {
                let mut c = client(addr, Duration::from_secs(3));
                c.write_all(format!("GET /echo/c{} HTTP/1.1\r\n\r\n", i).as_bytes()).unwrap();
                let mut resp = String::new();
                c.read_to_string(&mut resp).expect("response");
                assert!(resp.ends_with(&format!("\r\n\r\nc{}", i)), "client {} got: {:?}", i, resp);
            })
        })
        .collect();
    for c in clients {
        c.join().expect("client thread");
    }
    server.join().expect("serve should return");
}
