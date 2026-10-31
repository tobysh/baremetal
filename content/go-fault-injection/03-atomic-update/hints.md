Your current `Save` writes straight to `db`. Think about what is left in `db` if the write stops after 3 bytes.
===
Write `db.tmp` fully, then `Rename("db.tmp", "db")`. If `WriteFile` returns an error, do not rename.
===
`Load`: call `d.Remove("db.tmp")` first, ignoring its result. Then `ReadFile("db")`; `ok == false` means not found.
===
Use `decode`; if it reports failure return `nil, false, ErrCorrupt`. Do not return partial data when the checksum fails.
