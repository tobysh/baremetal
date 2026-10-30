Keep the supplied helpers and implement:

```go
func ExecuteBundle(bundle []byte, caps map[string]string,
                   maxOps int, maxTicks uint32) (Trace, error)
```

Call Unpack, then LoadCapsule, then Run on `image.Instructions[image.Entry:]`. Propagate any stage's error as Trace{},error. Preserve all input bytes and the capability map. maxOps applies to the executed suffix, including its HALT; maxTicks and the 4096-byte output limit retain their prior meaning. Do not return partial trace output on failure.

The supplied main builds `READ "note"; EMIT "done"; HALT`, wraps and packs it, and executes it with the public capability `note=public lab data`. Run shows its trace. Test varies packages, entry offsets, permitted data, instructions and resource limits. It also repackages corrupt capsules with valid outer checksums to ensure the inner validation still runs.

This is the final lesson. Its optional challenge adds trusted artifact identity and policy checks covering the whole image.


Public fixture (`sample.json`, with the envelope also in `sample.bundle.hex`):
```json
{
  "format": "BX01 containing BML1 bytecode",
  "bundle_hex": "4258303119001370b30930515e5f2213131e13d203e50011177d7c67761217777c7d7613",
  "caps": {
    "note": "public lab data"
  },
  "max_ops": 3,
  "max_ticks": 0,
  "expected": {
    "Outputs": [
      "public lab data",
      "done"
    ],
    "Reads": [
      "note"
    ],
    "Ticks": 0
  }
}
```
