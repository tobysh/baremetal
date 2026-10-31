Implement `Cases() []string`: at most 30 input strings for `Normalize`. Do not change `Normalize`; the tests use their own copy of the original and check yours still agrees with it on all your inputs.

To pass, for each of the fourteen hidden mutants there must be at least one of your inputs where the mutant's output differs from the original's. Inputs may repeat but each counts towards the limit of 30. The public example in `main` prints what `Normalize` does with a few inputs.

Suggestion: first write down every branch of `Normalize` and for each one the smallest input just inside it and just outside it.
