Read standard input: at most 500 lines, each at most 1000 characters.

A line is **well formed** when all of these hold:

- it has a first whitespace-separated token (the client address, at most 63 characters);
- it contains a pair of double quotes (the first two `"` on the line) enclosing exactly three whitespace-separated words: method, path, version (the path is at most 500 characters);
- after the closing quote there are exactly two tokens: a decimal status from 100 to 599, and a size that is `-` or only digits.

Any other line, including an empty line, is malformed.

Print first `2xx=A 3xx=B 4xx=C 5xx=D malformed=M`: the number of well-formed lines per status class (1xx lines are well formed but not counted in a class) and the number of malformed lines.

Then, for every address with two or more well-formed requests whose lowercased path contains `../`, `%2e%2e` or `/etc/passwd`, print `SUSPECT ADDRESS hits=N`. Each request counts once however many patterns it matches. Sort by hits descending, then by `strcmp` on the address. Malformed lines never count as hits. An address with a single matching request is not printed. Nothing else is printed.

The hidden tests use different logs: statuses 99, 100, 599 and 600, bad size tokens, requests with two or four words, empty lines, upper- and lowercase encodings, paths matching several patterns, one hit versus two, ties in hit counts and an empty input. Keep the program reading stdin and writing to stdout.
