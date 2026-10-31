`encoding/binary` has `binary.BigEndian.AppendUint32(b, v)` and `binary.BigEndian.Uint32(p)`.
===
Version: `v := min(local.MaxVer, remote.MaxVer)` and `lo := max(local.MinVer, remote.MinVer)`; if `v < lo` there is no overlap.
===
Features: loop over `featureSince` (`for bit, since := range featureSince`); enable `bit` when both offered it and `since <= v`. Finally `(local.Required | remote.Required) &^ enabled != 0` means something required is missing.
