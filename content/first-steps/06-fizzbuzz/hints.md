Start with a `for` loop that prints every number from 1 to 20. Get that working first, then add the rules.
===
Check the FizzBuzz case (divisible by both 3 and 5) *first*. A number divisible by both 3 and 5 is divisible by 15.
===
```c
if (i % 15 == 0) printf("FizzBuzz\n");
else if (i % 3 == 0) printf("Fizz\n");
```
…and two more cases.
