Write one pass for each rule and append to a slice, then sort once at the end. Keep the rules separate; a combined loop makes the interactions hard to see.
===
For beacons, collect indexes of emits per text and compute the gap between two events by summing the wait ticks in between (a prefix sum of wait ticks makes it one subtraction). Remember to skip texts listed in Heartbeats before looking at their gaps.
===
For ParseReport, decode with json.Decoder and DisallowUnknownFields into structs with pointer fields so that "absent" is distinguishable from zero, then confirm that a second Decode call returns io.EOF.
