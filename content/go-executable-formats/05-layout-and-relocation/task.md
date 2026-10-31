Implement `Layout` and `Relocate` in `starter.go` (resolution and the parser are provided).

`Layout(objs)` returns a `Plan`: `TextAddr = Base`, `TextSize` (end of the last text section relative to `Base`, without trailing padding), `DataAddr = alignUp(Base+TextSize, PageSize)`, `DataSize` (end of the last data section relative to `DataAddr`), `MemSize` (end of the last bss section relative to `DataAddr`; with no bss sections it equals `DataSize`), and the address of every section through `Addr(obj, sec)`. Each section start is aligned up to 4. If any section would end above `1<<20`, return an error wrapping `ErrTooBig`.

`Relocate(objs, res, plan)` returns an `Image` whose `Text` (`TextSize` bytes) and `Data` (`DataSize` bytes) hold the section contents at their addresses (gaps zero) with every relocation applied. Compute S from `res.Target` and `plan.Addr`. Range failures wrap `ErrRelocRange`. Do not modify the objects.

The tests include hand-computed layouts, random layouts, scribbled-over fields, negative and large addends, and an image ending exactly at the size limit.
