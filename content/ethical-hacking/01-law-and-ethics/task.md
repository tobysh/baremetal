**Part 1, quiz.** Implement `int quiz_answer(int n)` for `n` from 0 to 7. Return `1` if the statement is true, `0` if false.

0. A verbal "go ahead" from your manager is enough permission to test a customer's system.
1. A written scope list limits what you may test, as well as describing it.
2. If you accidentally find an open flaw in a system you do not own, you should log in and explore a bit to confirm it before reporting.
3. Testing machines you own, in your own home lab, normally needs nobody else's permission.
4. Good intentions are a reliable legal defence under computer-misuse laws.
5. Reporting a flaw privately to the owner and giving them time to fix it before publishing is coordinated disclosure.
6. If you meet a host that is not on the scope list during a test, you stop and ask the contact.
7. A signature from someone who does not control the system is valid permission to test it.

**Part 2.** Implement `int may_proceed(const Roe *r, const char *target, int day, const char *technique)`. Return `GO_AHEAD` (0) only if everything is in order, otherwise the **first** failing reason, checked in this order:

1. `STOP_NO_PERMISSION` if `r->owner_signed` is `0`.
2. `STOP_OUT_OF_SCOPE` if `target` matches no entry of `r->scope[0..nscope-1]`. An entry matches by exact text, or, if it starts with `*.`, any host that ends with the rest of the entry (so `*.lab.test` matches `a.lab.test` and `a.b.lab.test` but not `lab.test` and not `xlab.test`).
3. `STOP_OUTSIDE_WINDOW` if `day` is before `start_day` or after `end_day` (both inclusive).
4. `STOP_FORBIDDEN_TECHNIQUE` if `technique` equals any of `r->forbidden[0..nforbidden-1]`.

Many different engagements are tried, including empty scopes and look-alike host names.
