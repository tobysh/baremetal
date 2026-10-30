Write **`unsigned audit_response(const char *response)`**. The response is text: a status line, header lines separated by `\r\n` or `\n`, a blank line, then a body. Return the OR of these flags (the starter defines them):

- `BAD_SECURE` (1): any `Set-Cookie` header has no `Secure` attribute.
- `BAD_HTTPONLY` (2): any `Set-Cookie` header has no `HttpOnly` attribute.
- `BAD_SAMESITE` (4): any `Set-Cookie` header has no `SameSite` attribute with the value `Lax`, `Strict` or `None`.
- `BAD_HSTS` (8): no `Strict-Transport-Security` header with a `max-age=` directive of at least **15552000** seconds (180 days).
- `BAD_NOSNIFF` (16): no `X-Content-Type-Options` header whose value is `nosniff`.

Rules: header names, attribute names and the attribute values above are case-insensitive, and spaces around them are ignored. Cookie attributes are the `;`-separated parts after the first `;`; the name and value of the cookie itself never count. Only lines before the first blank line are headers. A response with no `Set-Cookie` header has no cookie flags. Several cookies are all checked, and one weak cookie is enough to set a flag. A header name must match exactly: `X-Strict-Transport-Security` is not HSTS.

Many different responses are tried.
