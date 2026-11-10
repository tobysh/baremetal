Implement `long add_two(long a, long b)` in assembly. It returns `a + b`.

By the System V calling convention, `a` arrives in `rdi` and `b` in `rsi`; leave the result in `rax` and finish with `ret`.

The checks also use fresh positive, negative and 64-bit inputs. Their sums stay within the signed `long` range.
