`find` is `users.iter().find(|u| u.id == id)`. For `manager_name`: `let u = find(users, id)?; let m = find(users, u.manager?)?; Some(m.name.as_str())`.
===
`display_name`: `u.nick.as_deref().filter(|n| !n.is_empty()).unwrap_or(&u.name).to_string()`. `as_deref` turns `Option<String>` into `Option<&str>` without moving it.
===
`parse_port`: `s.and_then(|t| t.parse::<u16>().ok()).filter(|&p| p >= 1024)`. `add_both`: `a.zip(b).map(|(x, y)| x + y)`. `pick_first_set`: `a.or(b).or(c).ok_or_else(|| "nothing set".to_string())`.
