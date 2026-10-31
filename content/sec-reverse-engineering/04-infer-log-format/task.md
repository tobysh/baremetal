Input is a file as hex digits on standard input (any whitespace between them). Parse it with the layout you inferred and print what the viewer printed: one `<timestamp> <LEVEL> <text>` line per record, then `records: <n>`. Level numbers 0 to 3 are named `DEBUG`, `INFO`, `WARN`, `ERROR`; any other level `n` prints as `L<n>`. Text bytes outside 32 to 126 print as `.`.

Damaged input, in this order of checking:

- fewer bytes than the fixed header: `error: truncated header`
- wrong magic: `error: bad magic`
- a version other than the one in the samples: `error: unsupported version <v>`
- a record whose fixed part or text runs past the end: `error: truncated record <i>` (numbered from 1). Earlier records are already printed. Print no `records:` line after an error.
- bytes left over after all declared records: print `warning: <k> trailing bytes` before the `records:` line.

Hidden tests include files with many records, a record count above 255, and each kind of damage.
