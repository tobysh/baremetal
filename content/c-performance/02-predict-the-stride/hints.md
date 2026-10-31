Run the program and compare the measured column with your formula by eye for a few strides. Where do they first disagree?
===
Every read below one line apart is on a new line only when the step is at least 16 ints. Below that, count lines: the last index read is `(reads - 1) * stride`, and its line is `index * 4 / 64`.
===
For the sets, the visited sets are `(k * stride_lines) % 16`. How many distinct values does that take? Think about `gcd(16, stride_lines)`.
