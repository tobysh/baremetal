Implement:

```go
var (
    ErrMalformed = errors.New("malformed manifest")
    ErrUntrusted = errors.New("untrusted signature")
    ErrDigest    = errors.New("payload digest mismatch")
    ErrExpired   = errors.New("manifest expired")
    ErrForbidden = errors.New("permission not allowed")
)
type Manifest struct {
    Name     string   `json:"name"`
    Digest   string   `json:"digest"`
    Perms    []string `json:"perms"`
    KeyID    string   `json:"key_id"`
    NotAfter uint64   `json:"not_after"`
}
type Signed struct{ Manifest, Sig []byte }
type Policy struct {
    Keys    map[string]ed25519.PublicKey
    Allowed map[string]bool
    Now     uint64
}
const Domain = "BM-MANIFEST-v1\x00"

func Admit(s Signed, payload []byte, p Policy) (Manifest, error)
```

Check in this order and return `Manifest{}` plus an error wrapping the named sentinel (`errors.Is` must work):

1. **ErrMalformed**: `s.Manifest` is one strict JSON object (no unknown fields, no trailing data); Name is 1 to 64 bytes; Digest is 64 lowercase hex characters; KeyID is non-empty; Perms has at most 8 entries, each non-empty, no duplicates.
2. **ErrUntrusted**: `KeyID` is in `p.Keys` (use it only as a lookup), that key is exactly `ed25519.PublicKeySize` bytes, the signature is exactly `ed25519.SignatureSize` bytes, and the signature verifies over `Domain + s.Manifest` (the raw received bytes).
3. **ErrDigest**: SHA-256 of `payload`, in lowercase hex, equals `Digest`.
4. **ErrExpired**: `p.Now <= NotAfter`.
5. **ErrForbidden**: every entry of Perms has `p.Allowed[perm] == true`.

Return the parsed Manifest on success. Never modify the inputs. Hidden tests use changed manifests, wrong keys, truncated and signed-without-domain signatures, and manifests that are authentic yet not permitted.
