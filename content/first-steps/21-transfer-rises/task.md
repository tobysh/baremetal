The input is any number of whole numbers separated by spaces or newlines, possibly none. Think of them as a series of readings, one after another.

A **rise** is a number that is strictly greater than the number immediately before it. The first number can never be a rise, because nothing comes before it.

Print how many rises there are, then the size of the biggest rise, which is how much that number exceeded the one before it. If nothing in the series rose, say so instead of printing a size. Use this layout, with the words exactly as shown:

```
rises: 2
biggest rise: 4
```

Every number and every difference fits in an `int`. Your program is tried on many different series, so it must work for whatever it is given.

The file starts with only `#include <stdio.h>`. Choose your own functions (or none) and write `main` yourself.
