Stdin: rules in the format of the previous lesson (no packets this time, at most 64 rules). For each rule J (1-based, in order) that is shadowed by some earlier rule, using the lowest-numbered earlier rule I that fully covers it, print `rule J shadowed by rule I (redundant)` if both have the same action, or `rule J shadowed by rule I (conflict)` otherwise. Print `no shadowed rules` if there are none.

Tests use hand-built and generated policies including non-normalised networks.

Details: a rule shadowed by several earlier rules is reported once, with the lowest-numbered covering rule; every rule that is shadowed gets its own line, in rule order; a rule is never reported as shadowed by a later rule; an identical earlier rule shadows it; `any` for protocol, network or ports means the widest value.
