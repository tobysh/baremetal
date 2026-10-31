Implement `Resolve`, `Lookup` and `Target` in `starter.go`.

`Resolve(objs)` first enters every defined (`Sec >= 0`) global symbol into the table, in object order then symbol order. A second definition of a name returns an error wrapping `ErrDuplicate` whose text contains the name in quotes (`"bump"`). Only when all definitions are in, check each undefined symbol, in the same order; a missing one returns an error wrapping `ErrUndefined`, naming the symbol. Duplicates are therefore always reported before undefined symbols. `Resolve(nil)` succeeds.

`Lookup(name)` finds a global. `Target(obj, sym)` returns the `Def` for symbol `sym` of object `obj`, or `ok == false` for an index out of range (including negatives).

Hidden tests try both object orders, repeated local names, locals named like globals, and several simultaneous problems.
