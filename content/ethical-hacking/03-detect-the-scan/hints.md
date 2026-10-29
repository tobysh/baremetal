Store the events as you read them: timestamp, source, action and port. When you read a new event, look back over the earlier events of the same source with `ts > t - 10`.
===
Use a small array of distinct ports for the window and test membership before adding. Remember to keep a per-source `already reported` flag so a source is printed only once.
