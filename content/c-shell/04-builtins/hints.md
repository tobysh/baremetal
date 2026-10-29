Start with `if (strcmp(argv[0], "cd") == 0) {...} else if (strcmp(argv[0], "exit") == 0) {...} else return 0;`. Count the arguments by walking `argv` until `NULL`.
===
For `cd`: pick `argv[1]` or `getenv("HOME")`, call `chdir`, and on `-1` use `fprintf(stderr, "cd: %s: %s\n", dir, strerror(errno))`.
===
For `exit N` check that every character after an optional `-` is a digit before calling `strtol`. Use `((v % 256) + 256) % 256` or `v & 255` to get the status byte.
