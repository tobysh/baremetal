package main

import (
	"fmt"
	"math/rand"
	"reflect"
	"testing"
)

func xk(n int64) Value { return Value{N: n, Valid: true} }

var xnull = Value{}

func xrow(k, v Value) Row { return Row{Key: k, Value: v} }

func xdesc(g Group) string {
	s := func(v Value) string {
		if !v.Valid {
			return fmt.Sprintf("NULL(N=%d)", v.N)
		}
		return fmt.Sprint(v.N)
	}
	return fmt.Sprintf("key=%s rows=%d count=%d sum=%s", s(g.Key), g.Rows, g.Count, s(g.Sum))
}

func xcheckGroups(t *testing.T, what string, got, want []Group) {
	t.Helper()
	if len(got) != len(want) {
		t.Fatalf("%s: got %d groups, want %d (group order is first appearance; empty input gives none)", what, len(got), len(want))
	}
	for i := range want {
		if got[i] != want[i] {
			t.Fatalf("%s: group %d is\n  %s\nwant\n  %s", what, i, xdesc(got[i]), xdesc(want[i]))
		}
	}
}

func TestAggregate_worked_example_from_the_lesson(t *testing.T) {
	rows := []Row{
		xrow(xk(1), xk(1)), xrow(xk(1), xnull), xrow(xnull, xk(5)),
		xrow(xnull, xnull), xrow(xk(2), xnull),
	}
	xcheckGroups(t, "lesson example", Aggregate(rows), []Group{
		{Key: xk(1), Rows: 2, Count: 1, Sum: xk(1)},
		{Key: xnull, Rows: 2, Count: 1, Sum: xk(5)},
		{Key: xk(2), Rows: 1, Count: 0, Sum: xnull},
	})
}

func TestAggregate_empty_input_gives_no_groups(t *testing.T) {
	if got := Aggregate(nil); len(got) != 0 {
		t.Fatalf("nil input: got %d groups, want 0", len(got))
	}
	if got := Aggregate([]Row{}); len(got) != 0 {
		t.Fatalf("empty input: got %d groups, want 0", len(got))
	}
}

func TestAggregate_all_NULL_values_give_NULL_sum_not_zero(t *testing.T) {
	rows := []Row{xrow(xk(4), xnull), xrow(xk(4), Value{N: 99}), xrow(xk(4), xnull)}
	got := Aggregate(rows)
	xcheckGroups(t, "all-NULL values (an invalid Value with N=99 must be ignored)", got,
		[]Group{{Key: xk(4), Rows: 3, Count: 0, Sum: xnull}})
}

func TestAggregate_a_sum_of_zero_is_still_a_valid_sum(t *testing.T) {
	rows := []Row{xrow(xk(1), xk(5)), xrow(xk(1), xk(-5)), xrow(xk(2), xk(0))}
	xcheckGroups(t, "sums that equal zero", Aggregate(rows), []Group{
		{Key: xk(1), Rows: 2, Count: 2, Sum: xk(0)},
		{Key: xk(2), Rows: 1, Count: 1, Sum: xk(0)},
	})
}

func TestAggregate_NULL_keys_share_one_group_whatever_their_N(t *testing.T) {
	rows := []Row{
		xrow(Value{N: 7}, xk(1)), xrow(xnull, xk(2)), xrow(Value{N: -3}, xk(4)),
		xrow(xk(0), xk(8)), xrow(xk(7), xk(16)),
	}
	xcheckGroups(t, "NULL keys", Aggregate(rows), []Group{
		{Key: xnull, Rows: 3, Count: 3, Sum: xk(7)},
		{Key: xk(0), Rows: 1, Count: 1, Sum: xk(8)},
		{Key: xk(7), Rows: 1, Count: 1, Sum: xk(16)},
	})
}

func TestAggregate_groups_in_first_appearance_order_with_negative_keys(t *testing.T) {
	rows := []Row{
		xrow(xk(9), xk(1)), xrow(xk(-2), xk(1)), xrow(xk(9), xk(1)),
		xrow(xk(3), xnull), xrow(xk(-2), xk(1)), xrow(xk(3), xk(10)),
	}
	xcheckGroups(t, "order", Aggregate(rows), []Group{
		{Key: xk(9), Rows: 2, Count: 2, Sum: xk(2)},
		{Key: xk(-2), Rows: 2, Count: 2, Sum: xk(2)},
		{Key: xk(3), Rows: 2, Count: 1, Sum: xk(10)},
	})
}

func TestAggregate_single_row_and_does_not_mutate_input(t *testing.T) {
	rows := []Row{xrow(Value{N: 5}, xk(3))}
	before := append([]Row(nil), rows...)
	xcheckGroups(t, "one NULL-key row", Aggregate(rows), []Group{{Key: xnull, Rows: 1, Count: 1, Sum: xk(3)}})
	if !reflect.DeepEqual(rows, before) {
		t.Fatalf("Aggregate modified its input: %v", rows)
	}
}

func TestAggregate_matches_reference_on_random_tables(t *testing.T) {
	r := rand.New(rand.NewSource(78))
	for n := 0; n < 160; n++ {
		rows := make([]Row, n)
		for i := range rows {
			// invalid values and keys get random N so unnormalised code fails
			k := Value{int64(r.Intn(7)), r.Intn(3) != 0}
			v := Value{int64(r.Intn(61) - 30), r.Intn(3) != 0}
			if !k.Valid {
				k.N = int64(r.Intn(5))
			}
			rows[i] = Row{k, v}
		}
		before := append([]Row(nil), rows...)
		want := []Group{}
		for _, x := range rows {
			found := -1
			for i, g := range want {
				if g.Key.Valid == x.Key.Valid && (!g.Key.Valid || g.Key.N == x.Key.N) {
					found = i
					break
				}
			}
			if found < 0 {
				k := x.Key
				if !k.Valid {
					k.N = 0
				}
				want = append(want, Group{Key: k})
				found = len(want) - 1
			}
			want[found].Rows++
			if x.Value.Valid {
				want[found].Count++
				want[found].Sum.Valid = true
				want[found].Sum.N += x.Value.N
			}
		}
		xcheckGroups(t, fmt.Sprintf("random table with %d rows", n), Aggregate(rows), want)
		if n > 0 && !reflect.DeepEqual(rows, before) {
			t.Fatalf("input changed for table of %d rows", n)
		}
	}
}
