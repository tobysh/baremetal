Inside `swap` you receive addresses. To read or change the values living at those addresses, use `*a` and `*b`.
===
Swapping needs a spare place to hold one value while you overwrite it, just like swapping the contents of two cups needs a third cup. Make a temporary `int`.
===
```c
int tmp = *a;
*a = ...;   // put the value from b here
*b = ...;   // put the saved value here
```
