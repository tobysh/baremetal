Stdin: rules in the format of the previous lesson (no packets this time, at most 64 rules). For each rule J (1-based, in order) that is shadowed by some earlier rule, using the lowest-numbered earlier rule I that fully covers it, print `rule J shadowed by rule I (redundant)` if both have the same action, or `rule J shadowed by rule I (conflict)` otherwise. Print `no shadowed rules` if there are none.

Tests use hand-built and generated policies including non-normalised networks.
