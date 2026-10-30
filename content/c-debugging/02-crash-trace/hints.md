Use the address in the trace. The fault was at 0x8 and `cents` is at offset 8, so which pointer was NULL?
===
`find_item` returns NULL when nothing matches. `price_of` uses the result without looking at it.
===
In `price_of`, return -1 when `find_item` gives NULL. In `order_total`, add the price only when it is not negative.
