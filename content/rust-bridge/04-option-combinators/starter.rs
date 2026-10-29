struct User {
    id: u32,
    name: String,
    manager: Option<u32>,
    nick: Option<String>,
}

fn find(users: &[User], id: u32) -> Option<&User> {
    None
}

fn manager_name(users: &[User], id: u32) -> Option<&str> {
    None
}

fn display_name(u: &User) -> String {
    String::new()
}

fn parse_port(s: Option<&str>) -> Option<u16> {
    None
}

fn add_both(a: Option<i32>, b: Option<i32>) -> Option<i32> {
    None
}

fn pick_first_set(a: Option<u8>, b: Option<u8>, c: Option<u8>) -> Result<u8, String> {
    Err(String::new())
}

// Try it with Run.
fn main() {
    let users = vec![
        User { id: 1, name: "Ada".to_string(), manager: None, nick: None },
        User { id: 2, name: "Bob".to_string(), manager: Some(1), nick: Some("bobby".to_string()) },
    ];
    println!("{:?}", manager_name(&users, 2));
    println!("{}", display_name(&users[1]));
    println!("{:?}", parse_port(Some("8080")));
    println!("{:?}", add_both(Some(1), Some(2)));
    println!("{:?}", pick_first_set(None, Some(4), Some(5)));
}
