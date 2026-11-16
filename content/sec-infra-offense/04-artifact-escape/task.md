Implement `EscapeArtifact(lab Lab, port int, root, protected string) (Finding, bool)`. The signature and types are fixed.

`root` and `protected` are clean absolute POSIX paths with no `.` or `..` components. `root` is a non-root directory with no trailing slash; `protected` is a file outside `root`.

Send **one** request: `GET /artifact` on `port`, with `Body` set to an absolute requested path, empty `Token`. The vulnerable server accepts a request whose raw text starts with `root + "/"`, then resolves `..` segments and serves the file at the resolved path. The patched server resolves first and checks the boundary afterwards. So your payload must start with `root + "/"` as written, use `..` to climb from `root` up to `/`, and then name `protected`.

Return `Finding{Port: port, Evidence: body}` and `true` only if the reply has status 200 and a `Body` beginning `LAB-PROOF:`. Otherwise return the zero `Finding` and `false`. A reflected path or a plain 200 is not evidence.

Rules: exactly one request; no URL encoding; no local filesystem access.
