Plan before you type: this is a loop that reads until `scanf` stops returning 1, with a count and a biggest value that start at 0 and only change when a number rises.
===
You need a variable holding the previous number, and another saying whether there is a previous number yet. Compare only when there is one, then store the current number as the new previous number.
===
A rise has `x > previous`. Its size is `x - previous`. Add 1 to the count, and replace the biggest if this size is larger or if it is the first rise.
===
After the loop, print `rises: ` and the count. For the second line, print `biggest rise: none` when the count is 0, otherwise the biggest size.
