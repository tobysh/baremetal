Implement:

1. `int score_finding(const Finding *f)` using the formula above, including clamping.
2. `const char *severity_name(int score)` returning exactly `"critical"`, `"high"`, `"medium"`, `"low"` or `"info"`.
3. `void triage(Finding *list, int n)` sorts the array in place: higher `score_finding` first, ties by higher (clamped) `exposure` first, remaining ties by `strcmp(id)` ascending.

Many random findings are scored and sorted; some have out-of-range fields.
