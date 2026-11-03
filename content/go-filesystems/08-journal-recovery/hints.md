Validate the whole transaction before applying its first record.
===
Retain the commit marker on a failed replay; clearing it early loses recovery information.
===
Redo writes assign values rather than incrementing, making replay safe.
