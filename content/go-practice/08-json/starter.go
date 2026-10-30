package main

import "fmt"

type Item struct {
	Name     string
	Price    float64
	Tags     []string
	internal int
}

func Encode(items []Item) (string, error) {
	return "", nil
}

func Pretty(items []Item) (string, error) {
	return "", nil
}

func Decode(s string) ([]Item, error) {
	return nil, nil
}

func Total(s string) (float64, error) {
	return 0, nil
}

// Try it with Run.
func main() {
	s, err := Encode([]Item{{Name: "pen", Price: 1.5, Tags: []string{"office"}}})
	fmt.Println(s, err)
}
