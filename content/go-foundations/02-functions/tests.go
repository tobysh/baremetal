package main

import "testing"

func TestDivMod_exact(t *testing.T) {
	q, r := DivMod(12, 4)
	if q != 3 || r != 0 {
		t.Errorf("DivMod(12, 4) = %d, %d; want 3, 0", q, r)
	}
}

func TestDivMod_remainder(t *testing.T) {
	q, r := DivMod(17, 5)
	if q != 3 || r != 2 {
		t.Errorf("DivMod(17, 5) = %d, %d; want 3, 2", q, r)
	}
}

func TestDivMod_by_zero_does_not_crash(t *testing.T) {
	q, r := DivMod(5, 0)
	if q != 0 || r != 0 {
		t.Errorf("DivMod(5, 0) = %d, %d; want 0, 0", q, r)
	}
}

func TestMinMax_several_values(t *testing.T) {
	min, max := MinMax(3, 9, -2, 7)
	if min != -2 || max != 9 {
		t.Errorf("MinMax(3, 9, -2, 7) = %d, %d; want -2, 9", min, max)
	}
}

func TestMinMax_all_negative(t *testing.T) {
	min, max := MinMax(-8, -3, -5)
	if min != -8 || max != -3 {
		t.Errorf("MinMax(-8, -3, -5) = %d, %d; want -8, -3", min, max)
	}
}

func TestMinMax_single_value(t *testing.T) {
	min, max := MinMax(42)
	if min != 42 || max != 42 {
		t.Errorf("MinMax(42) = %d, %d; want 42, 42", min, max)
	}
}

func TestMinMax_no_arguments(t *testing.T) {
	min, max := MinMax()
	if min != 0 || max != 0 {
		t.Errorf("MinMax() = %d, %d; want 0, 0", min, max)
	}
}

func TestMinMax_spread_slice(t *testing.T) {
	xs := []int{5, 1, 4}
	min, max := MinMax(xs...)
	if min != 1 || max != 5 {
		t.Errorf("MinMax(xs...) = %d, %d; want 1, 5", min, max)
	}
}
