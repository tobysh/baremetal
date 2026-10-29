Keep one cursor into `src.char_indices().collect::<Vec<_>>()`; offsets come from that vector, not from the cursor number.
===
Gather an entire number or identifier before deciding its token. For two-character operators, inspect the next character before advancing by one or two.
