---
title: JSON with Struct Tags
summary: Turn Go values into JSON and back with encoding/json, and control the field names and omissions with tags.
minutes: 30
difficulty: medium
tags: json, structs
id: go-practice/json
mode: harness
---
JSON is how most programs talk to each other. Go's `encoding/json` converts between JSON text and your structs.

```go
type Item struct {
	Name  string  `json:"name"`
	Price float64 `json:"price"`
	Tags  []string `json:"tags,omitempty"`
}
```

The text in backquotes after a field is a **struct tag**. `json:"name"` makes the JSON key `name` instead of the Go field name `Name`; `omitempty` leaves the key out when the value is empty (`""`, `0`, `nil`, an empty slice).

## Encoding and decoding

```go
b, err := json.Marshal(item)           // []byte: {"name":"pen","price":1.5}
b, err = json.MarshalIndent(item, "", "  ") // pretty-printed

var it Item
err = json.Unmarshal([]byte(text), &it) // note the pointer
```

Rules worth knowing:

- Only **exported** (capitalised) fields are encoded. A field called `secret` is silently ignored, which is also how you keep something out of the output.
- `json:"-"` excludes an exported field.
- `Unmarshal` ignores keys it has no field for, and leaves fields missing from the JSON at their zero value.
- It returns an error for malformed JSON or a value of the wrong type (`"price": "free"` into a `float64`).

## Slices and maps

`Unmarshal` into a `[]Item` reads a JSON array; into a `map[string]int` reads an object. Marshalling a `nil` slice gives `null`, while an empty non-nil slice gives `[]`: another reason to return `[]T{}` rather than `nil`.
