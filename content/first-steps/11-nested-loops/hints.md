Triangle: the outer loop runs `row` from 1 to 4; the inner loop prints `row` stars; then print `"\n"`.
===
Table: the outer loop is the row (1 to 3), the inner loop is the column (1 to 3), and each cell is `row * col`.
===
To avoid a trailing space: `if (col > 1) printf(" ");` before printing the number.
