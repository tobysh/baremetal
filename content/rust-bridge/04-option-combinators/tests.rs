fn sample() -> Vec<User> {
    let mk = |id: u32, name: &str, manager: Option<u32>, nick: Option<&str>| User {
        id,
        name: name.to_string(),
        manager,
        nick: nick.map(|s| s.to_string()),
    };
    vec![
        mk(1, "Ada", None, None),
        mk(2, "Bob", Some(1), Some("bobby")),
        mk(3, "Cyd", Some(2), Some("")),
        mk(4, "Dee", Some(99), None),
        mk(7, "Eli", Some(3), Some("e")),
    ]
}

#[test]
fn find_users() {
    let u = sample();
    assert_eq!(find(&u, 3).map(|x| x.name.as_str()), Some("Cyd"), "id 3");
    assert!(find(&u, 5).is_none(), "id 5 does not exist");
    assert_eq!(find(&u, 7).map(|x| x.id), Some(7), "ids are not positions");
}

#[test]
fn manager_names() {
    let u = sample();
    assert_eq!(manager_name(&u, 2), Some("Ada"), "Bob reports to Ada");
    assert_eq!(manager_name(&u, 3), Some("Bob"), "Cyd reports to Bob");
    assert_eq!(manager_name(&u, 7), Some("Cyd"), "Eli reports to Cyd");
    assert_eq!(manager_name(&u, 1), None, "Ada has no manager");
    assert_eq!(manager_name(&u, 4), None, "manager 99 is missing");
    assert_eq!(manager_name(&u, 50), None, "unknown user");
}

#[test]
fn display_names() {
    let u = sample();
    assert_eq!(display_name(&u[0]), "Ada", "no nickname");
    assert_eq!(display_name(&u[1]), "bobby", "nickname wins");
    assert_eq!(display_name(&u[2]), "Cyd", "empty nickname is ignored");
    assert_eq!(display_name(&u[4]), "e", "short nickname still counts");
}

#[test]
fn ports() {
    assert_eq!(parse_port(Some("8080")), Some(8080), "valid");
    assert_eq!(parse_port(Some("1024")), Some(1024), "boundary");
    assert_eq!(parse_port(Some("1023")), None, "just below");
    assert_eq!(parse_port(Some("80")), None, "privileged");
    assert_eq!(parse_port(Some("http")), None, "not a number");
    assert_eq!(parse_port(Some("70000")), None, "too big for u16");
    assert_eq!(parse_port(None), None, "missing");
}

#[test]
fn adding_options() {
    assert_eq!(add_both(Some(2), Some(3)), Some(5), "both");
    assert_eq!(add_both(Some(-4), Some(4)), Some(0), "cancel");
    assert_eq!(add_both(None, Some(3)), None, "left missing");
    assert_eq!(add_both(Some(3), None), None, "right missing");
    assert_eq!(add_both(None, None), None, "both missing");
}

#[test]
fn first_set() {
    assert_eq!(pick_first_set(Some(1), Some(2), Some(3)), Ok(1), "a wins");
    assert_eq!(pick_first_set(None, Some(2), Some(3)), Ok(2), "b next");
    assert_eq!(pick_first_set(None, None, Some(0)), Ok(0), "zero is still a value");
    assert_eq!(pick_first_set(None, None, None), Err("nothing set".to_string()), "all empty");
}
