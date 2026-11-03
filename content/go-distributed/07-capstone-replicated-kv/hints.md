Validate the commit boundary before indexing the log.
===
Decode all newly committed commands into a temporary slice before changing either map.
===
Track IDs separately from keys: two different requests may legitimately overwrite one key.
===
Advance Applied even when an entry is a duplicate request.
