Read lines from standard input. A claim line is `C<n>|<text>`. An evidence line is `E<n>|<kind>|<claim id>|<stance>` with kind `static`, `dynamic` or `report` and stance `supports` or `contradicts`. Lines have at most 200 characters, there are at most 64 claims, and other lines are ignored.

Evidence for a claim id that has not been defined *earlier* in the input is ignored, and for each such line print `warning: evidence <E id> references unknown claim <C id>` as soon as it is read (before the claim lines below).

Then print one line per claim in the order given: `<C id>: <label> - <text>`, where the label is one of `confirmed`, `supported (static)`, `supported (dynamic)`, `assumption`, `refuted` or `disputed` by the rules in the lesson. Finish with `confirmed=<a> supported=<b> assumption=<c> refuted=<d> disputed=<e>`, where `supported` counts both supported labels. With no claims print only the last line (and any warnings).
