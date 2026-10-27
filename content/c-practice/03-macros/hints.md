Imagine the preprocessor pasting text. Write out what `SQUARE(1 + 2)` becomes with your current macro, then add brackets until it means what you want.
===
Bracket every use of an argument (`(x)`) and bracket the whole macro body too: `((x) * (x))`. For `MAX`, `((a) > (b) ? (a) : (b))`.
===
For `SWAP_INT`, rename the temporary so it cannot clash: `int swap_tmp_ = (a);`. For `STRINGIFY`, the `#` operator goes in front of the parameter name: `#x`. For `ARRAY_LEN`, divide the whole size by the size of one item.
