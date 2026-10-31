Implement `Assess(ev []Evidence, claims []Claim) []Verdict`, one verdict per claim in order, with `Status` chosen by the first matching rule:

1. `"unsupported"` if `Cites` is empty or any cited ID is not in `ev`.
2. `"contradicted"` if **any** observed evidence in `ev` (cited or not) has the claim's `Subject` and `Attr` with a different `Value`.
3. `"supported"` if a cited **observed** item matches `Subject`, `Attr` and `Value` exactly.
4. `"reported"` if a cited non-observed item matches exactly.
5. `"inferred"` if the claim lists at least one entry in `Assumes` (the cited evidence exists but does not match directly).
6. Otherwise `"unsupported"` (an undeclared leap).

Public example: evidence `E1` (observed) says `web1.listener=4444`; a claim of the same value citing `E1` is supported; the same claim citing only a ticket (`T1`, not observed) with that value is reported.

Whether an assumption is sensible cannot be machine-graded; only that it is declared.
