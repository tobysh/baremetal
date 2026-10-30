Marshal the two login fields with encoding/json; avoid hand-built JSON.
===
A candidate token may belong to a nonadmin role. Check the protected marker before declaring success.
