Decode the hex payload into bytes first, rejecting odd lengths and bad digits.
===
Compute `end = depth ? min(len, offset+depth) : len`; try each start from `offset` while `start + patlen <= end`.
