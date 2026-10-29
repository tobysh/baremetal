struct User {
    id: u32,
    name: String,
    manager: Option<u32>,
    nick: Option<String>,
}

fn find(users: &[User], id: u32) -> Option<&User> {
    users.iter().find(|u| u.id == id)
}

fn manager_name(users: &[User], id: u32) -> Option<&str> {
    let u = find(users, id)?;
    let m = find(users, u.manager?)?;
    Some(m.name.as_str())
}

fn display_name(u: &User) -> String {
    u.nick
        .as_deref()
        .filter(|n| !n.is_empty())
        .unwrap_or(&u.name)
        .to_string()
}

fn parse_port(s: Option<&str>) -> Option<u16> {
    s.and_then(|t| t.parse::<u16>().ok()).filter(|&p| p >= 1024)
}

fn add_both(a: Option<i32>, b: Option<i32>) -> Option<i32> {
    a.zip(b).map(|(x, y)| x + y)
}

fn pick_first_set(a: Option<u8>, b: Option<u8>, c: Option<u8>) -> Result<u8, String> {
    a.or(b).or(c).ok_or_else(|| "nothing set".to_string())
}

fn main() {
    println!("{:?}", add_both(Some(1), Some(2)));
}
