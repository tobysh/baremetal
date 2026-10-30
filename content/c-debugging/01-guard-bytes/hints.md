Compare the "after" dump with the struct layout, byte by byte. Which field did the extra letters land in?
===
The loop stops only when it meets the end of the *input*. It never asks how much room `name` has.
===
Stop copying after `sizeof r->name - 1` characters, write `'\0'` at that position, and count the rest of `s` to get the return value.
