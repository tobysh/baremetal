Run the program: the nested loops make 12 million comparisons on the big balanced case. How many does merge need at most?
===
Merge works when the lengths are similar. When one array is much shorter, binary search per element of the short array is far cheaper. Compare the two bounds from the lesson to choose.
===
Write `merge(a,na,b,nb)` and `search(small,ns,big,nbig)`. In `count_common`, swap so `small` is the shorter, compute `ns * ceil_log2(nbig+1)` and `na + nb`, and call the cheaper one.
