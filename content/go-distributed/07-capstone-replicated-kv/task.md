Implement `Store.Apply(log, commit)`. Indices are counts: apply entries `[Applied, commit)` and leave the uncommitted suffix untouched. Decode `Entry.Cmd` as JSON `Command`. Reject an empty ID, empty key, malformed JSON, a decreasing commit count or a count beyond the log. An invalid call must leave all maps and `Applied` unchanged.

Apply valid commands in log order. The first command for each request ID writes its value and records its ID; later uses of that ID do nothing. Different IDs may update the same key. After success set `Applied = commit`. Repeating a commit is safe. Do not modify the log.

Hidden tests replicate commands through the supplied leader, withhold a majority, retry requests, advance commit in stages and reconstruct a fresh store by replaying the committed prefix.
