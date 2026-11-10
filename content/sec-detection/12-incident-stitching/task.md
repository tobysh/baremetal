Stdin: first line `G`, then at most 1000 unsorted events `<ts> <host> <user> <message...>`. Sort by timestamp (stable). For each user start a new burst when the gap to that user's previous event is greater than `G`. Print one line per burst: `<user> <first>-<last> n=<count> hosts=<h1,h2,...>` (distinct hosts in first-seen order within the burst) with ` MULTI-HOST` appended if there is more than one host. Order bursts by first timestamp, then by user name (`strcmp`).

Tests use several gap sizes, including bursts exactly `G` seconds apart.

Details: a gap is `next.ts - previous.ts` for the same user after sorting, and a gap equal to `G` stays in the burst; `G` may be 0 (only same-second events join); hosts and users contain no spaces and a burst has at most 8 distinct hosts; the message text is ignored; output is one line per burst, and nothing is printed for empty input after the `G` line.
