Check the version and the exact body length first, then walk the body with an index. At each field verify that two header bytes remain, and that the value length fits in what remains.
===
Keep a small set of tags already seen. Decide a field's meaning with a switch on the tag and the version together; the default branch handles unknown tags by looking only at the high bit.
===
Describe is the easy one: never format the Token field itself. Print its length. The tests look for the token, its hex, its base64 and every single-byte XOR of it.
