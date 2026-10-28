Add the tags exactly as in the lesson: `` `json:"name"` ``, `` `json:"price"` `` and `` `json:"tags,omitempty"` ``. `internal` stays lowercase and untagged, so it is skipped.
===
`Encode`: `if items == nil { items = []Item{} }` first (a nil slice would encode as `null`), then `b, err := json.Marshal(items)` and `return string(b), err`.
===
`Pretty` is the same with `json.MarshalIndent(items, "", "  ")`.
===
`Decode`: `var items []Item; if err := json.Unmarshal([]byte(s), &items); err != nil { return nil, err }`. `Total` = `Decode` then add up `Price`.
