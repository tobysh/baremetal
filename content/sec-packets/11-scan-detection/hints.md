Parse each line with `sscanf` into a time, two names, an integer port and an outcome word, and add a trailing `%1s` field: if it parses, the line had too many fields and you should skip it. Skip the line if the count is not exactly five, or the port or outcome is out of range.
===
Keep one record per client: its name, a list of (server, port) targets, and counters for open, closed and filtered. Find the client by name (add it if new) before touching its targets.
===
Before adding a target, search that client's list for the same server and port. If it is already there, ignore the line completely so the first outcome stays and the counters are not touched.
===
Flag a client with `n >= 5 && closed + filtered > open`. Gather their indexes in an array and sort it: larger `n` first, and on a tie `strcmp` of the names ascending. Print `no scans detected` when the array is empty.
