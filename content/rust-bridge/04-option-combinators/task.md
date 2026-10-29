The starter defines `struct User { id: u32, name: String, manager: Option<u32>, nick: Option<String> }`. Complete:

- `fn find(users: &[User], id: u32) -> Option<&User>`: the user with that id.
- `fn manager_name(users: &[User], id: u32) -> Option<&str>`: the name of that user's manager. `None` if the user does not exist, has no manager, or the manager id is not in the list. Use `?` or `and_then`.
- `fn display_name(u: &User) -> String`: the nickname if there is one that is not empty, otherwise the real name. Use `as_deref`, `filter` and `unwrap_or`.
- `fn parse_port(s: Option<&str>) -> Option<u16>`: parse the text as a port number and accept it only if it is at least 1024. Missing text, text that is not a number, or too small a port gives `None`.
- `fn add_both(a: Option<i32>, b: Option<i32>) -> Option<i32>`: the sum if both are present, otherwise `None`. Use `zip`.
- `fn pick_first_set(a: Option<u8>, b: Option<u8>, c: Option<u8>) -> Result<u8, String>`: the first of `a`, `b`, `c` that is `Some`, or `Err("nothing set".to_string())`. Use `or` and `ok_or`.
