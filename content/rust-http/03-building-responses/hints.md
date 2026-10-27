`reason_phrase` is a `match` on `status` with one arm per code and `_ => "Unknown"` at the end.
===
For `build_response`, one `format!` call is enough. It needs five values: the status, the reason, the content type, the length and the body. Put `\r\n` inside the format string where each line ends.
===
Use `body.len()` for the length, not `body.chars().count()`. The reason comes from `reason_phrase(status)`. The blank line is the final `\r\n\r\n` before `{}` for the body.
