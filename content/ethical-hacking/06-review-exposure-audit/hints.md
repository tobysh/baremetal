Start with the scan loop from the port scanner lesson. Inside it, once you know the port is open, do the allow-list test and then the banner test, appending to `out` through a small helper that respects `cap`.
===
To split the banner, use `strchr(b, '/')`. Compare the part before it with each advisory's product using `strncmp` and a length check (or copy it into a small buffer).
