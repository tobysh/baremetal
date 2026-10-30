Decode the known cookie, compute HMAC for each candidate and compare the signature bytes.
===
Once a candidate matches, sign desired rather than reusing the known signature. Use RawURLEncoding, not padded base64.
