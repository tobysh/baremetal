Skip every line that does not contain `sshd[`. Then look for the exact text ` Failed password for ` or ` Accepted password for ` with `strstr`, and parse after it. Anything else is noise.
===
After ` Failed password for `, skip the literal `invalid user ` only if it is really there (`strncmp` with its length). Then `sscanf(p, "%63s from %63s", user, ip)` gives the user and the address.
===
Keep one struct per address: total failures, a list of distinct users, and the accepted user (empty until set). Find the address in a small table, adding a new row when it is new.
===
When you see an acceptance, record the user only if that address already has at least five failures and nothing recorded yet. At the end collect the qualifying rows into an index array and sort it by failures descending, then `strcmp` on the address.
