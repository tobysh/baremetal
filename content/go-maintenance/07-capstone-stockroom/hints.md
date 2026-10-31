Begin with the read side: make `Load` accept 4 fields and store a `reserved` value on the item. Check that the old fixtures still load.
===
Next make `Save` conditional: append `,reserved` only when it is positive. Compare against a file written by the old code.
===
`Reserve`, `Release`, `Fulfil` and `Available` are small. Put the invariant in one place: `0 <= reserved <= qty`. `Adjust` must keep it.
===
Do not touch `Low` and `Value`. Resist the urge to use `Available` in them: the task says they stay physical.
===
`Upgrade` can reuse the line parser that `Load` uses: parse each record line, then format it canonically. Keep a separate map of seen skus for the duplicate check.
===
Idempotence follows from using the same formatter that `Save` uses for single items.
