Think about the order the vulnerable server works in: it checks the prefix on the raw text, then resolves `..`. Your payload must pass the first check and only reach the protected file after the second step.
===
The payload begins with `root + "/"`. Each `../` removes one directory, so you need as many as `root` has components to get back to `/`.
===
Count components with `strings.Split(strings.Trim(root, "/"), "/")`, repeat `"../"` that many times, and then append `protected` without its leading slash.
===
Do not run `path.Clean` on the payload. After sending, accept only status 200 and a body with the prefix `LAB-PROOF:`; a body like `requested path: ...` is just an echo.
