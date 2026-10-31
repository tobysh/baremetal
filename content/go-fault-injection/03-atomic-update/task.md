Implement `Save` and `Load` against the `Disk` interface in the starter. The file names are `db` and `db.tmp`.

- `Save(d Disk, data []byte) error` writes `encode(data)` to `db.tmp`, then renames it to `db`. It stops at the first error and returns it. On a failed write it makes a best-effort `Remove("db.tmp")` and still returns the write's error.
- `Load(d Disk) (data []byte, found bool, err error)` first removes any `db.tmp` (best effort, ignore its error). If `db` does not exist it returns `found=false`. Otherwise it decodes `db` and returns the data, or `ErrCorrupt` if the checksum or length is wrong.
- Empty data is valid and distinct from "not found".

The tests crash every save at every operation with several tear sizes and check that committed data is never lost and a torn result is never returned.
