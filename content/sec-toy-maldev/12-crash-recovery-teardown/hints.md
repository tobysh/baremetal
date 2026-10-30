Write one helper that returns the map for a Kind (or nil for an unknown one). Use it in Apply, Recover and Audit.
===
In Apply the order is: crash check, append the journal entry, crash check, write. Compute the crash points from i, not from a counter you update afterwards.
===
Recover must check all entries first, then loop from len-1 down to 0. Set j.Entries = nil only after the loop.
===
Audit: for every path in either map compare presence and value, then sort the resulting strings.
