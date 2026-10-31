Checkpoint 1 first. Track which cfg keys you used in a set, and compute the entry as len(program) at the moment you see the entry step.
===
For the byte offset, add up the encoded size of each instruction before the entry: EMIT and READ take 2 plus the text length, WAIT takes 3.
===
Deploy: loop over the two (path, content) pairs; for each, check absence, append to the journal, then write. Do not use defer for the journal.
===
RunProject: run the stages in a helper function that returns (Report, error), then call Teardown once in the outer function whatever the helper returned.
