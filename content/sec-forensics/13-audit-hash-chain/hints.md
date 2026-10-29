Split each line with `strchr` on `|`, requiring exactly three bars. Check SEQ is digits only, does not start with 0, and is short enough for an `unsigned long`.
===
Build `PREV|SEQ|ACTOR|ACTION` with `snprintf` into a buffer, hash it byte by byte with `h=(h^b)*1099511628211ULL`, and print with `%016llx`.
