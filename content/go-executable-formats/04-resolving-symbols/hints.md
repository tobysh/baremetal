Keep `globals map[string]Def`. Only symbols with `Sec >= 0 && Bind == BindGlobal` go in.
===
Use two loops: one over all objects for definitions, then one over all objects for undefined symbols. Doing both in one loop reports the wrong problem when a definition comes later.
===
`Target`: bounds-check `obj` and `sym`; if the symbol has `Sec >= 0` return its own location, else `r.Lookup(s.Name)`.
===
Error text: `fmt.Errorf("symbol %q defined in objects %d and %d: %w", name, a, b, ErrDuplicate)`.
