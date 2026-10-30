Implement `VerifyRelease(pub ed25519.PublicKey, manifest, sig []byte, files map[string][]byte) error`.

Order of checks: public key length and signature (`ErrBadSignature`), then manifest syntax (`ErrBadManifest`), then for each listed file in manifest order missing (`ErrMissingFile`) or wrong size or digest (`ErrFileMismatch`), then any unlisted file (`ErrExtraFile`). The error variables are supplied. The function must not modify its inputs and must never panic, including for a nil or short key or signature.

`main` signs a small public manifest with a throwaway key, verifies it, and then verifies a tampered copy. The hidden tests use other keys, truncated and appended manifests, signed-but-hostile manifests (traversal names, duplicates, bad sizes, missing final newline) and tampered or extra files. A manifest that is both badly signed and badly formed must report the signature, not the syntax.
