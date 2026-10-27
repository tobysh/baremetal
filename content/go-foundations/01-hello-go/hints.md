Declare the values first: `language := "Go"`, `year := 2009`, `static := true`, `pi := 3.14159`.
===
Each line is one `fmt.Printf` call: `fmt.Printf("Language: %s\n", language)`. Use `%d` for the year and `%t` for the bool.
===
For the last line use `%.2f`, which rounds 3.14159 to `3.14`: `fmt.Printf("Pi is about %.2f\n", pi)`. Don't forget the `\n` at the end of every format string.
