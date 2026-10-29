Stdin: first line `G`, then at most 1000 unsorted events `<ts> <host> <user> <message...>`. Sort by timestamp (stable). For each user start a new burst when the gap to that user's previous event is greater than `G`. Print one line per burst: `<user> <first>-<last> n=<count> hosts=<h1,h2,...>` (distinct hosts in first-seen order within the burst) with ` MULTI-HOST` appended if there is more than one host. Order bursts by first timestamp, then by user name (`strcmp`).

Tests use several gap sizes, including bursts exactly `G` seconds apart.
