Parse each request's fields by name; the values never contain spaces.
===
Check the origin by exact string comparison (`strcmp`), never `strstr` or `strncmp`.
