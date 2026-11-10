Group by registered domain. For every query you need to find the domain (the last two labels, lowercased), the subdomain (everything before that) and whether the record type is TXT or NULL.
===
Keep a table per domain with Q, txt, U and S. To count U and S only for distinct subdomains, remember each (domain, subdomain) pair you have seen, and update U and S only the first time. Scan from the right of the name for the second dot to find the cut point.
===
If a domain with only a few distinct names shows up, you added S for repeats. If a flagged domain is missing, check you compare `S >= 20 * U` and count the dots inside a multi-label subdomain. Sort with two keys: higher U first, then name.
===
Finding the last two labels on a different example (the last label, the TLD, of a name):
```c
const char *dot = strrchr(name, '.');
const char *tld = dot ? dot + 1 : name;
```
