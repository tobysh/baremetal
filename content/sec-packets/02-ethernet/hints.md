Start with the guard: reject NULL pointers and `n < 14` before reading anything. The EtherType is `(p[12] << 8) | p[13]`; cast to `unsigned` before shifting so promotion rules cannot surprise you.
===
Build the result in a local `Ethernet e` and copy it to `*out` only at the end. That is the easiest way to honour "do not change `*out` on failure".
===
When the first type is 0x8100, check `n >= 18` before touching bytes 16 and 17. The tag control information at 14..15 can be skipped, and the payload offset becomes 18.
===
Do not loop: only one tag is unwrapped. If the inner type is 0x8100 as well, report it as the type and leave the decision to the caller.
