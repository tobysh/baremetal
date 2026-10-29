Walk the entries in order and store each in a `map[string]Entry`. Because later entries overwrite earlier ones in the map, you end with the newest per key without any extra logic.
===
Collect the map's keys into a slice and `sort.Strings` it. Then build the result by looking each key up; skip entries with `Delete` set when `dropTombstones` is true.
===
Copy the `Value` bytes of each kept entry if you want the result to share nothing with the input. At the least, never write into `entries[i]`.
===
`CompactLog`: `entries, err := DecodeEntries(log)`; on error return `nil, err`; otherwise `return EncodeEntries(Compact(entries, true)), nil`.
