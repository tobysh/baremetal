package main

import "fmt"

func main() {
	language := "Go"
	year := 2009
	static := true
	pi := 3.14159

	fmt.Printf("Language: %s\n", language)
	fmt.Printf("Released: %d\n", year)
	fmt.Printf("Static typing: %t\n", static)
	fmt.Printf("Pi is about %.2f\n", pi)
}
