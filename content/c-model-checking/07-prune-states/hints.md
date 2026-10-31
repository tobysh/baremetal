Store nodes in an array: the state, the index of the parent node and the thread id that led here, and the depth. The queue is just the array with a `head` index.
===
Before appending a new state, look it up in a hash table of `memcmp`-equal states. If it is already there, skip it.
===
Classify a node when it leaves the queue (`head`). The first one that is not `V_OK` is the answer; walk `parent` links to fill `sched` from the end.
===
`states` is the final `count` of nodes stored.
