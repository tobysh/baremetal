Read the `# sources` section into a small table, then each event: parse the number, the source name, and take the rest of the line (without the trailing newline) as the message. Store the true time and the input index.
===
Sort with a stable method: insertion sort works for 1000 events, or compare `(true_ts, index)`. Map the first word of the message to a phase number with a few `strcmp` calls.
