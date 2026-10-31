Read STATUS once in `flash_unlock` and test the LOCKED bit. Write KEY1 and KEY2 only inside that `if`, then read STATUS again to see whether it worked.
===
The program loop is `for (int attempt = 0; attempt < 3; attempt++)`. Each iteration: wait, three register writes, wait, read STATUS.
===
After a failed attempt write `ERR` (bit 1) to STATUS before looping, otherwise the controller drops the next CMD forever. Return -3 only after the loop.
