Implement `Review(oldText, newText string, policy Policy) ([]Finding, error)`. `Pkg`, `Finding`, `Policy`, `ParseLock` and `Version` comparison helpers are supplied. Return a nil or empty slice when nothing is found.

`main` reviews a small public pair of lockfiles. Hidden tests use many other pairs and compare the complete ordered findings, including pairs where one package yields several findings, names that are exactly one edit apart, names shorter than four characters, `_`/`-` variants, and malformed input.
