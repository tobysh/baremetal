Decode with json.NewDecoder, DisallowUnknownFields, then a second Decode must return io.EOF. Check field shapes next; hex.DecodeString accepts uppercase, so compare against the lowercase re-encoding or check the characters yourself.
===
Wrap errors with fmt.Errorf("%w: detail", ErrMalformed) so errors.Is works.
===
Build the message with append([]byte(Domain), s.Manifest...) so you do not alias the caller's slice. Check len(pub) and len(sig) before calling ed25519.Verify.
