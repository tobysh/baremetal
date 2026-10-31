package main

type Event struct {
	Time                  int64
	Actor, Action, Object string
}

type Item struct {
	Name, Kind string
	Since      int64
}

func Scope(startCred string, startTime int64, events []Event, stored map[string][]string) []Item {
	return nil
}

func main() {}
