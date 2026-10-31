List the branches: separators stripped, `00` prefix, `+` prefix, extension suffix, national 0-prefix, ten digits, eleven digits starting with 1, and the length limits.
===
For every comparison (`<`, `>=`, a length limit) choose the exact boundary value, and the values on both sides of it.
===
Look at what is *stripped* as well as what is kept: spaces, dashes, dots, parentheses, upper and lower case `X`/`EXT`, surrounding whitespace.
===
Look at what returns `""`: too short, too long, stray letters, an extension that is too long, extension with no digits.
===
Inputs should be short and distinct. Thirty is enough: about two per branch.
