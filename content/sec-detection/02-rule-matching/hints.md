Split the work in two: first parse every `RULE` line into name, field, op and value, then for each event loop over the rules in order and test each one.
===
Look up the field with a helper that returns found/not found, and skip the rule immediately when it is not found. Only then compare, using `strcmp`, `strstr`, `strncmp` and a tail comparison for the four operators.
===
If `endswith` crashes or matches shorter fields, you compare `f + fl - vl` without checking `fl >= vl`. If events of the wrong kind trigger a rule, a missing field is being treated as an empty value. If the rule list prints in the wrong order, you are looping over events inside rules instead of rules inside events.
===
Tail comparison on a different example, "does the path end with `.log`":
```c
size_t pl = strlen(path), sl = strlen(".log");
int ends = pl >= sl && strcmp(path + pl - sl, ".log") == 0;
```
