Declare an integer variable, then read into it with `scanf("%d", &number)`. The `&` tells `scanf` where to store the value.
===
Check the return value: `if (scanf("%d", &number) != 1) { return 1; }` stops the program if it could not read an integer.
===
After reading successfully, use `printf("%d\n", number * 2);` to print twice the value and a newline.
