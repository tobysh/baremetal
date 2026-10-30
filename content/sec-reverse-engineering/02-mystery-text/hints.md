Scan each line once, finding where the current run ends, then decide how to print it based on its length.
===
The threshold is "three or more"; shorter runs are printed as they are, character by character.
===
Use `printf("%c%zu", ch, run)` for the long form so the count is printed in full.
