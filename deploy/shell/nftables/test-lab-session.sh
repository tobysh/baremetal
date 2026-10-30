#!/bin/sh
# Behavioural test of lab-session.nft. Needs unprivileged user namespaces (no root, no host network is touched): everything
# runs inside `unshare -Urnm`, with veth pairs standing in for the Firecracker taps. Prints PASS/FAIL lines, exits 1 on any FAIL.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
if [ "${BM_INNER:-}" != 1 ]; then
  exec env BM_INNER=1 unshare -Urnm sh "$0"
fi
fail=0
ok() { echo "PASS $*"; }
bad() { echo "FAIL $*"; fail=1; }
mount -t tmpfs tmpfs /run; mkdir -p /run/netns
for n in gl gt; do ip netns add $n; done
ip link add tap-l type veth peer name eth0 netns gl
ip link add tap-t type veth peer name eth0 netns gt
ip addr add 10.77.0.1/30 dev tap-l; ip addr add 10.77.1.1/30 dev tap-t
ip link set lo up; ip link set tap-l up; ip link set tap-t up
sysctl -qw net.ipv4.ip_forward=1 net.ipv4.conf.all.rp_filter=1 net.ipv4.conf.default.rp_filter=1
ip -n gl addr add 10.77.0.2/30 dev eth0; ip -n gl link set lo up; ip -n gl link set eth0 up
ip -n gt addr add 10.77.1.2/30 dev eth0; ip -n gt link set lo up; ip -n gt link set eth0 up
ip -n gl route add default via 10.77.0.1; ip -n gt route add default via 10.77.1.1
nft -f "$here/lab-session.nft"
# target services: 80 (allowed port), 9999 (not in TARGET_PORTS)
# Needs python3 (no netcat dependency).
srv='import socket,sys
s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1); s.bind(("0.0.0.0",int(sys.argv[1]))); s.listen(8)
while True: s.accept()[0].close()'
ip netns exec gt python3 -I -c "$srv" 80 & ip netns exec gt python3 -I -c "$srv" 9999 & ip netns exec gl python3 -I -c "$srv" 4444 &
sleep 1
cli='import socket,sys
s=socket.socket(); s.settimeout(2)
if len(sys.argv)>3: s.bind((sys.argv[3],0))
s.connect((sys.argv[1],int(sys.argv[2]))); sys.exit(0)'
tcp() { ip netns exec "$1" timeout 4 python3 -I -c "$cli" "$2" "$3" ${4:-} >/dev/null 2>&1; }
tcp gl 10.77.1.2 80 && ok "learner -> target:80 allowed" || bad "learner -> target:80 should work"
tcp gl 10.77.1.2 9999 && bad "learner -> target:9999 reached" || ok "learner -> target:9999 (port not listed) blocked"
tcp gt 10.77.0.2 4444 && bad "target -> learner new connection reached" || ok "target -> learner new connection blocked"
tcp gl 10.77.0.1 80 && bad "learner -> namespace address reached" || ok "learner -> namespace/host-side address blocked"
tcp gl 169.254.169.254 80 && bad "metadata address reached" || ok "learner -> 169.254.169.254 blocked"
tcp gl 192.168.1.1 80 && bad "RFC1918 reached" || ok "learner -> 192.168.1.1 blocked"
tcp gl 8.8.8.8 53 && bad "internet reached" || ok "learner -> 8.8.8.8:53 blocked"
ip netns exec gl timeout 3 ping -c1 -W1 10.77.1.2 >/dev/null 2>&1 && bad "ICMP to target allowed" || ok "ICMP to target blocked"
ip netns exec gl timeout 3 ping -c1 -W1 10.77.0.1 >/dev/null 2>&1 && bad "ICMP to gateway allowed" || ok "ICMP to gateway blocked"
# spoofed source from learner: add a second address and use it
ip -n gl addr add 10.99.0.9/32 dev eth0
tcp gl 10.77.1.2 80 10.99.0.9 && bad "spoofed source passed" || ok "spoofed source address blocked"
dropped=$(nft list chain inet bm_lab forward | awk '/everything else/ {print}' | grep -o 'packets [0-9]*' | head -1 | cut -d' ' -f2)
[ "${dropped:-0}" -gt 0 ] && ok "default-drop counter incremented ($dropped packets)" || bad "default-drop counter is zero"
kill $(jobs -p) 2>/dev/null || true
[ "$fail" = 0 ] && echo "ALL PASS" || { echo "FAILURES"; exit 1; }
