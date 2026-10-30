# Trying to become root or gain capabilities. Everything here is a refusal check.
bm_info privesc.id "$(id)"
[ "$(id -u)" = 1000 ] && [ "$(id -g)" = 1000 ] && bm_pass privesc.ids "uid/gid 1000" || bm_fail privesc.ids "$(id)"
case "$(id -G)" in 1000) bm_pass privesc.groups "no supplementary groups";; *) bm_fail privesc.groups "groups: $(id -G)";; esac
caps=""; while read -r k v; do case "$k" in CapInh:|CapPrm:|CapEff:|CapBnd:|CapAmb:) [ "$v" = 0000000000000000 ] || caps="$caps $k$v";; NoNewPrivs:) nnp=$v;; esac; done </proc/self/status
[ -z "$caps" ] && bm_pass privesc.caps "all capability sets empty" || bm_fail privesc.caps "non-empty:$caps"
[ "${nnp:-0}" = 1 ] && bm_pass privesc.no_new_privs "set" || bm_fail privesc.no_new_privs "NoNewPrivs=${nnp:-?}"
bm_denied privesc.setpriv-root setpriv --reuid=0 --regid=0 --clear-groups true
bm_denied privesc.setpriv-caps setpriv --inh-caps +all --ambient-caps +all true
bm_denied privesc.setpriv-bounding setpriv --bounding-set +all true
bm_denied privesc.setpriv-sysadmin setpriv --inh-caps +sys_admin --ambient-caps +sys_admin true
# A setuid-root copy: the learner owns the copy, so the bit can only ever give back uid 1000 (and no_new_privs ignores it).
cp /bin/id /tmp/bm-sid 2>/dev/null && bash -c 'chmod u+s /tmp/bm-sid' 2>/dev/null
r=$(/tmp/bm-sid -u 2>/dev/null); [ "$r" = 1000 ] && bm_pass privesc.setuid-copy "setuid copy still runs as 1000" || bm_fail privesc.setuid-copy "setuid copy ran as '$r'"
rm -f /tmp/bm-sid
bm_cannot_open_r privesc.shadow /etc/shadow
bm_cannot_open_w privesc.passwd /etc/passwd
bm_denied privesc.ls-root ls /root
bm_denied privesc.setuid-0 bash -c 'exec setpriv --reuid=0 id -u'
bm_done
