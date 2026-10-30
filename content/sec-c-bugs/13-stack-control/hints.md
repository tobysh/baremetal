Capacity arithmetic can overflow: check offset against SIZE_MAX - 8 before adding.
===
Write byte i of win as (win >> (8*i)) & 0xff. Do not copy the host representation or use strlen.
===
The repaired copy must validate the logical buffer size before writing any bytes. The whole frame's physical capacity is not the local buffer's capacity.
