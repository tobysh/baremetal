package main

// DivMod returns the quotient and remainder of a / b ((0, 0) if b is 0).
func DivMod(a, b int) (int, int) {
	if b == 0 {
		return 0, 0
	}
	return a / b, a % b
}

// MinMax returns the smallest and largest argument ((0, 0) if there are none).
func MinMax(nums ...int) (min, max int) {
	if len(nums) == 0 {
		return
	}
	min, max = nums[0], nums[0]
	for _, n := range nums[1:] {
		if n < min {
			min = n
		}
		if n > max {
			max = n
		}
	}
	return
}
