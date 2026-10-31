Implement `Emit`, `LinkMap`, `Link` and `LinkAndRun` in `starter.go`. Parsing, resolution, layout, relocation, `ParseExe`, `Describe`, `RunExe` and `WriteObject` are provided.

`Emit(img, entry)`: `entry` must be inside `[TextAddr, TextAddr+TextSize)`, otherwise `ErrEntry`. Output: 12-byte header (`TEXE`, version 1, segment count, two zero bytes, entry), then the segment table, then the text bytes, then the data bytes. Segment 0 is text (flags R+X = 5, address `TextAddr`, file and memory size `TextSize`). Segment 1 exists only when `Plan.MemSize > 0` (flags R+W = 6, address `DataAddr`, file size `DataSize`, memory size `MemSize`). Each segment's file offset is where its bytes sit in the output.

`LinkMap(objs, plan)`: all defined global symbols as `MapEntry{Name, Addr}`, final addresses, sorted by address then name.

`Link(objs, entry)`: parse (error text starts `object N:` and wraps the cause), `Resolve`, `Layout`, `Relocate`, find the entry (a name that is not a defined global is `ErrEntry` and its message contains the name), `Emit` (an entry that is not in text, such as a data symbol, is also `ErrEntry`).

`LinkAndRun(objs, entry, maxSteps)`: `Link`, `ParseExe` + `Describe`, `RunExe`. A link error returns `nil` and the error. A runtime fault or step-limit error returns the `Run` (with output printed so far) together with the error.

The tests link randomly sized two-object programs in both orders, compare bytes and map with the reference model, compare output with values computed in Go, and exercise every failure.
