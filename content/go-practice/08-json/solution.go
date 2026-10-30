package main

import (
	"encoding/json"
	"fmt"
)

type Item struct {
	Name     string   `json:"name"`
	Price    float64  `json:"price"`
	Tags     []string `json:"tags,omitempty"`
	internal int
}

func Encode(items []Item) (string, error) {
	if items == nil {
		items = []Item{}
	}
	b, err := json.Marshal(items)
	return string(b), err
}

func Pretty(items []Item) (string, error) {
	if items == nil {
		items = []Item{}
	}
	b, err := json.MarshalIndent(items, "", "  ")
	return string(b), err
}

func Decode(s string) ([]Item, error) {
	var items []Item
	if err := json.Unmarshal([]byte(s), &items); err != nil {
		return nil, err
	}
	return items, nil
}

func Total(s string) (float64, error) {
	items, err := Decode(s)
	if err != nil {
		return 0, err
	}
	sum := 0.0
	for _, it := range items {
		sum += it.Price
	}
	return sum, nil
}

// Try it with Run.
func main() {
	s, err := Encode([]Item{{Name: "pen", Price: 1.5, Tags: []string{"office"}}})
	fmt.Println(s, err)
}
