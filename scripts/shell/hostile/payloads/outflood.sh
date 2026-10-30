# Output flood from inside the guest: the broker must cut the session at its output cap (output_limit) and
# stop reading when the web side is slow. Nothing is checked in-guest; the driver asserts on the host.
bm_info outflood.start "flooding"
bm_done
line=$(printf 'X%.0s' {1..1000})
while :; do printf '%s\n%s\n%s\n%s\n' "$line" "$line" "$line" "$line"; done
