Keep three things together: a pointer to the numbers, how many you have read, and how many fit (the capacity). Read with `while (scanf("%d", &x) == 1)`.
===
Before storing a number, if `count == capacity`, call `realloc` with a bigger size (double it, starting from something like 4). Check that `realloc` did not return `NULL`, and keep the new pointer.
===
After reading, print from `count - 1` down to 0. Then count how many `data[i]` satisfy `(long long)data[i] * count > sum`, where `sum` was added up as a `long long`.
===
Handle the empty case without touching `data[0]`, and `free` the block before returning.
