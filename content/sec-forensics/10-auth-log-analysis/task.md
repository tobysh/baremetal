Read stdin (at most 1000 lines, 256 source IPs, 256 unique usernames per IP; names/IPs at most 63 characters). Count only sshd `Failed password for [invalid user ]USER from IP ...`. For IPs with >=5 failures print `IP failures=N users=U`, appending ` ACCEPTED as USER` for the first sshd password acceptance after that IP reached five failures. Ignore publickey and other events. Sort by failures descending, then strcmp(IP). If none qualify print `no brute force detected`. One newline per row.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
