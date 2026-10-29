Read the year first: `int year; scanf("%d", &year);`.
===
Use the rule from the lesson inside the `if`: `(year % 4 == 0 && year % 100 != 0) || year % 400 == 0`.
===
Print with `printf("%d is a leap year\n", year);` in the `if` branch and `... is not a leap year` in the `else`.
