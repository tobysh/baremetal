#!/bin/sh
# Mirrors sandboxArgs() in src/lib/runner.ts (no Rust toolchain installed). usage: sandbox.sh <workdir> cmd...
W=$1; shift
exec bwrap --unshare-all --die-with-parent --new-session --ro-bind /usr /usr --symlink usr/lib /lib --symlink usr/lib64 /lib64 --symlink usr/bin /bin --ro-bind /etc/ld.so.cache /etc/ld.so.cache --proc /proc --dev /dev --tmpfs /tmp --bind "$W" /work --chdir /work --clearenv --setenv PATH /usr/bin --setenv HOME /work --setenv LANG C.UTF-8 -- /usr/bin/prlimit --as=268435456 --cpu=5 --nproc=$(( $(ps -u $(id -u) --no-headers | wc -l) + 0 + 48 )) --fsize=4194304 --nofile=64 --core=0 -- "$@"
