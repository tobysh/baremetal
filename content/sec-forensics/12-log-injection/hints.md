Produce each piece into a small 4 byte array first, so you know its length before deciding to copy it.
===
Keep a `stopped` flag. Always add the piece length to the total, but copy only while `!stopped && w + len < cap`; set `stopped` the first time that test fails.
