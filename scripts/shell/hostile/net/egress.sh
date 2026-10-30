# REQUIRES THE LAB-NETWORK SETUP: a NIC on a per-session tap in its own netns with nftables default-drop
# (docs/interactive-shell-isolation.md "Network"). The broker's VMs currently have NO NIC; without one this
# script SKIPs, and the driver treats that as a failure only when BM_LAB_NETWORK=yes.
# Expected result for EVERY probe: blocked. Header variables are prepended by driver.mts (or set by hand):
#   BM_HOST_ADDRS="10.200.0.1 192.0.2.10"   addresses of the host (all its interfaces, incl. the tap's gateway side)
#   BM_OTHER_ADDRS="10.200.1.2"             addresses another concurrent session (or its target) would have
#   BM_CANARY_PORT=39999                    TCP+UDP listener the driver runs on the host; ANY hit is a failure
#   BM_STRICT=1                             treat a fast failure (RST/ICMP/reject) toward host classes as FAIL
: "${BM_HOST_ADDRS:=}" "${BM_OTHER_ADDRS:=}" "${BM_CANARY_PORT:=39999}" "${BM_STRICT:=0}"
nics=""; for n in /sys/class/net/*; do n=${n##*/}; [ "$n" = lo ] || nics="$nics $n"; done
if [ -z "$nics" ]; then bm_skip net.nic "no non-loopback interface: lab-network setup not configured"; bm_done; exit 0; fi
bm_info net.interfaces "$nics"
for n in $nics; do read -r a </sys/class/net/$n/address; read -r s </sys/class/net/$n/operstate; bm_info "net.if:$n" "mac=$a state=$s"; done
bm_info net.routes "$(while read -r l; do printf '%s; ' "$l"; done </proc/net/route)"
pkts() { local n l; for n in $nics; do while read -r l; do case "$l" in *"$n:"*) set -- ${l#*:}; printf '%s:rx=%s,tx=%s ' "$n" "$2" "$10";; esac; done </proc/net/dev; done; }
before=$(pkts)

probe_tcp() { # name host port class(host|other|ext)
  local p i=0 rc
  ( exec 3<>"/dev/tcp/$2/$3" ) 2>/dev/null & p=$!
  while kill -0 "$p" 2>/dev/null && [ "$i" -lt 8 ]; do bm_sleep 0.25; i=$((i + 1)); done
  if kill -0 "$p" 2>/dev/null; then kill -9 "$p" 2>/dev/null; wait "$p" 2>/dev/null; bm_pass "$1" "dropped, no answer in 2 s: tcp $2:$3"; return; fi
  wait "$p"; rc=$?
  if [ "$rc" = 0 ]; then bm_fail "$1" "CONNECTED to tcp $2:$3"
  elif [ "$BM_STRICT" = 1 ] && [ "${4:-ext}" != ext ]; then bm_fail "$1" "fast failure toward $4 address $2:$3 (reject/RST/ICMP): expected a silent drop"
  else bm_pass "$1" "no connection (fast failure: no route, reject or RST) tcp $2:$3"; fi
}
probe_udp() { # name host port class
  local got=""
  exec 4<>"/dev/udp/$2/$3" 2>/dev/null || { bm_pass "$1" "cannot even open udp socket to $2:$3"; return; }
  printf '\x12\x34\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x07example\x03com\x00\x00\x01\x00\x01' >&4 2>/dev/null
  read -r -t 2 -u 4 got 2>/dev/null && bm_fail "$1" "GOT A UDP REPLY from $2:$3" || bm_pass "$1" "no udp reply from $2:$3"
  exec 4>&- 2>/dev/null
}
run() { "$@" & }

for h in $BM_HOST_ADDRS; do
  for p in 22 53 80 443 3000 5000 8080 27017 "$BM_CANARY_PORT"; do run probe_tcp "net.host:$h:$p" "$h" "$p" host; done
  run probe_udp "net.host-udp53:$h" "$h" 53 host
  run probe_udp "net.host-canary-udp:$h" "$h" "$BM_CANARY_PORT" host
done
for h in $BM_OTHER_ADDRS; do for p in 22 80 5000; do run probe_tcp "net.other-session:$h:$p" "$h" "$p" other; done; done
# Default gateway (hex little-endian in /proc/net/route) is the host side of the tap: also off limits.
while read -r _ dest gw _; do
  [ "$dest" = 00000000 ] || continue
  g=$(printf '%d.%d.%d.%d' 0x${gw:6:2} 0x${gw:4:2} 0x${gw:2:2} 0x${gw:0:2}); bm_info net.gateway "$g"
  for p in 22 53 80 443 "$BM_CANARY_PORT"; do run probe_tcp "net.gateway:$p" "$g" "$p" host; done
  run probe_udp net.gateway-dns "$g" 53 host; run probe_udp net.gateway-dhcp "$g" 67 host
done </proc/net/route
# Link-local and cloud metadata endpoints.
for t in 169.254.169.254:80 169.254.169.254:443 169.254.170.2:80 169.254.1.1:80 100.100.100.200:80 192.0.0.192:80 fd00:ec2::254:80 fe80::1:80; do
  h=${t%:*}; p=${t##*:}; run probe_tcp "net.linklocal:$t" "$h" "$p" host
done
# RFC1918, CGNAT, loopback-alikes.
for h in 10.0.0.1 10.255.255.254 172.16.0.1 172.31.255.254 192.168.0.1 192.168.1.1 192.168.122.1 100.64.0.1; do
  for p in 22 80 443; do run probe_tcp "net.rfc1918:$h:$p" "$h" "$p" host; done
done
# The internet.
for t in 1.1.1.1:443 1.1.1.1:80 8.8.8.8:53 8.8.8.8:443 9.9.9.9:853 93.184.216.34:80 '2606:4700:4700::1111:443'; do h=${t%:*}; p=${t##*:}; run probe_tcp "net.internet:$t" "$h" "$p" ext; done
for h in 1.1.1.1 8.8.8.8 9.9.9.9; do run probe_udp "net.internet-dns:$h" "$h" 53 ext; done
run probe_udp net.internet-ntp 162.159.200.1 123 ext
wait
# Broadcast / multicast / DHCP / ARP. bash cannot set SO_BROADCAST, so broadcast sends are refused by the kernel (EACCES);
# multicast sends do leave the socket. What reaches the tap must be checked on the host (README: tcpdump + nft counters).
for t in 255.255.255.255:67 255.255.255.255:137 224.0.0.251:5353 239.255.255.250:1900 224.0.0.1:9 ff02::1:9 ff02::fb:5353; do
  h=${t%:*}; p=${t##*:}
  if ( exec 5<>"/dev/udp/$h/$p" && printf 'bm-hostile-broadcast-probe' >&5 ) 2>/dev/null; then bm_info "net.bcast:$t" "send accepted by the guest kernel (must not appear beyond the tap; check host capture)"; else bm_pass "net.bcast:$t" "send refused by the guest kernel"; fi
done
arp=""; while read -r l; do arp="$arp [$l]"; done </proc/net/arp
bm_info net.arp-table "$arp"
after=$(pkts)
bm_info net.counters "before: $before | after: $after"
bm_info net.reminder "host-side: canary hits and tcpdump on the tap decide the DROP-vs-REJECT and broadcast questions"
bm_done
