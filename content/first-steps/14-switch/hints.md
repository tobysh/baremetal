Read the number into an `int day`, then `switch (day) { ... }`.
===
Give each day its own case that prints the name and ends with `break`, so you can print the name there.
===
Print `weekday` or `weekend` in a second `switch`, with `case 1: case 2: case 3: case 4: case 5:` stacked above one `printf`.
===
Handle invalid days with `default:` in the first switch, and make sure the second part doesn't print for them (return from `main` there).
