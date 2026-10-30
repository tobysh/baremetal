package main

// SumTo returns 1 + 2 + ... + n (0 if n <= 0).
func SumTo(n int) int {
	total := 0
	for i := 1; i <= n; i++ {
		total += i
	}
	return total
}

// Collatz counts the steps needed to reach 1 from n (n >= 1).
func Collatz(n int) int {
	steps := 0
	for n != 1 {
		if n%2 == 0 {
			n /= 2
		} else {
			n = 3*n + 1
		}
		steps++
	}
	return steps
}

// DayKind returns "weekend", "weekday" or "unknown".
func DayKind(day string) string {
	switch day {
	case "sat", "sun":
		return "weekend"
	case "mon", "tue", "wed", "thu", "fri":
		return "weekday"
	default:
		return "unknown"
	}
}

// IndexOf returns the index of the first target in xs, or -1.
func IndexOf(xs []int, target int) int {
	for i, v := range xs {
		if v == target {
			return i
		}
	}
	return -1
}

// SumEvensUntil adds the even numbers of xs, stopping at the first
// number greater than limit.
func SumEvensUntil(xs []int, limit int) int {
	total := 0
	for _, v := range xs {
		if v > limit {
			break
		}
		if v%2 != 0 {
			continue
		}
		total += v
	}
	return total
}
