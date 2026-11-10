Read standard input: 1 to 1000 lines, each at most 511 characters, in the format `UnixSeconds source message`. `UnixSeconds` is a nonnegative integer that fits `time_t`. `source` has no spaces. The message is the rest of the line after the whitespace that follows the source (it may contain runs of spaces, which must be preserved). The last line may lack a newline. Every line is valid.

Sort the events by time with a **stable** sort (equal times keep their input order), then print:

1. For each event, in order: `YYYY-MM-DDTHH:MM:SSZ [source] message` where the time is UTC.
2. Immediately before an event, if its time minus the previous event's time is at least 3600 seconds, a line `-- quiet for Hh MMm --`: the whole hours (no padding) and the remaining whole minutes with two digits. Smaller gaps print nothing.
3. A final line `N events over Hh MMm`: the number of events and the time from the first to the last event in the same format (hours unpadded, minutes two digits).

No blank lines and no other text. The hidden tests use different inputs: ties at the same second, gaps of exactly 3600 and 3599 seconds, a span of more than 99 hours, the epoch, dates past 2038, already reversed input and messages with repeated spaces. Keep the program reading from stdin and writing to stdout.
