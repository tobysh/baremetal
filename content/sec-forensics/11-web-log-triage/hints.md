Read each line with `fgets` and take the address with `sscanf("%63s")`. Find the first and second `"` with `strchr`; if either is missing the line is malformed.
===
End the string at the second quote and parse the request with `sscanf("%15s %511s %15s %1s")`: it must return exactly 3 (a fourth word would fill the last conversion). The text after the closing quote is the status and size.
===
After the quote, `sscanf(" %d %31s %1s")` must return exactly 2. Check the status range, then check the size is `-` or all digits. Only after all that increment the status class counter.
===
Lowercase the path in a copy and test it with `strstr` for the three patterns. Keep a small table of addresses with hit counters, then sort an index array by hits descending and `strcmp` on the address before printing those with 2 or more hits.
