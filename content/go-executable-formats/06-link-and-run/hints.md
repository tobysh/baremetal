Link: a loop with `fmt.Errorf("object %d: %w", i, err)` around `ParseObject`, then the stages in order. Keep each stage's error wrapped with `%w`; the tests use `errors.Is`.
===
The entry address is `plan.Addr(d.Obj, d.Sec) + d.Off` where `d, ok := res.Lookup(entry)`. If `!ok`, return an error wrapping `ErrEntry`.
===
Emit: build `[]byte` with `binary.LittleEndian.AppendUint32`. The first segment's file offset is `12 + 20*nseg`; the next segment's offset adds the previous file size.
===
LinkMap: iterate every object's symbols; keep `Sec >= 0 && Bind == BindGlobal`; address is `plan.Addr(i, s.Sec) + s.Value`; sort with `sort.Slice` on address then name.
===
LinkAndRun: copy the `Linked` value into the `Run`, then `run.Output, err = RunExe(...)` and return `run, err` even when `err != nil`.
