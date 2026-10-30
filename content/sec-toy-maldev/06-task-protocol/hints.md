Use json.Decoder.DisallowUnknownFields, then require the second Decode to return io.EOF.
===
Track a private next sequence and private replies; every error returns the original last and nil replies. Guard against MaxUint64 before adding one.
