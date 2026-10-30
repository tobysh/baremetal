#!/bin/sh
# Prepare a dedicated host for the interactive-shell microVMs. Run as root, once per host and again after upgrades.
#
#   install-host.sh [--dry-run] [--allow-virtualized] [--firecracker-tarball FILE] [--no-download]
#
#   --dry-run              print every action and run only the read-only checks (works as a normal user)
#   --allow-virtualized    continue on a VM host (nested virtualisation: slower and a weaker boundary; see the doc)
#   --firecracker-tarball  use a local copy of firecracker-v1.17.0-x86_64.tgz instead of downloading it
#
# Idempotent: every step checks current state first. Installs and verifies; it does not build the kernel or rootfs
# (scripts/shell/build-kernel.sh, build-rootfs.sh), and it does not enable or start any service.
# Layout and rationale: docs/shell-deployment.md.
set -eu

FC_VERSION=v1.17.0
FC_ARCH=x86_64
FC_TARBALL_NAME=firecracker-$FC_VERSION-$FC_ARCH.tgz
FC_URL=https://github.com/firecracker-microvm/firecracker/releases/download/$FC_VERSION/$FC_TARBALL_NAME
# From the release's own .sha256.txt, fetched 30 October 2026 and cross-checked against the SHA256SUMS inside the tarball.
FC_TARBALL_SHA256=06094a1108ae9e82aa4c23a775aa92758f53f1175d422270d9d6162cb9ade558

LIBEXEC=/usr/local/libexec/baremetal-shell
SHARE=/usr/local/share/baremetal-shell
STATE=/var/lib/baremetal-shell
ETC=/etc/baremetal-shell
# Group the web service must join to reach the broker; group the broker joins to reach the launcher.
G_CLIENT=baremetal-shell-client
G_LAUNCH=baremetal-shell-launch
U_BROKER=baremetal-shell-broker
# Reserved uid/gid range for Firecracker processes (no accounts behind it). Must match SHELL_VM_UID in the launcher unit.
VM_UID_MIN=300000
VM_UID_MAX=365535

dry=0 allow_virt=0 tarball=
while [ $# -gt 0 ]; do
  case $1 in
    --dry-run) dry=1; shift ;;
    --allow-virtualized) allow_virt=1; shift ;;
    --firecracker-tarball) tarball=${2:?needs a file}; shift 2 ;;
    -h|--help) sed -n '2,13p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
problems=0

say() { printf '%s\n' "$*"; }
warn() { printf 'WARN  %s\n' "$*" >&2; }
bad() { printf 'FAIL  %s\n' "$*" >&2; problems=$((problems + 1)); }
die() { printf 'install-host: %s\n' "$*" >&2; exit 1; }
# run CMD ARGS...: print in dry-run mode, execute otherwise.
run() {
  if [ "$dry" = 1 ]; then printf 'DRY   %s\n' "$*"; else "$@"; fi
}

if [ "$dry" = 0 ] && [ "$(id -u)" != 0 ]; then die "must run as root (use --dry-run to preview as a normal user)"; fi

# ---- 1. Host checks (read-only, run in dry-run too) ---------------------------------------------------------------
say "== host checks"
[ "$(uname -m)" = x86_64 ] || bad "x86_64 only (Firecracker $FC_VERSION artefacts and the guest kernel config are x86_64)"
if [ -e /dev/kvm ]; then say "ok    /dev/kvm exists"; else bad "/dev/kvm is missing: no hardware virtualisation (enable VT-x/AMD-V in firmware, load kvm_intel/kvm_amd)"; fi
grep -Eq '(vmx|svm)' /proc/cpuinfo 2>/dev/null && say "ok    CPU advertises vmx/svm" || warn "no vmx/svm flag in /proc/cpuinfo (expected on a VM host without nested virtualisation, or if disabled in firmware)"
if command -v systemd-detect-virt >/dev/null 2>&1; then
  virt=$(systemd-detect-virt 2>/dev/null || true)
  if [ -n "$virt" ] && [ "$virt" != none ]; then
    if [ "$allow_virt" = 1 ]; then warn "running under $virt (nested virtualisation): weaker isolation and slower; accepted by --allow-virtualized"
    else bad "running under $virt: nested virtualisation is not accepted for production (pass --allow-virtualized for a test host)"; fi
  else say "ok    bare metal (systemd-detect-virt: none)"; fi
fi
[ "$(stat -fc %T /sys/fs/cgroup 2>/dev/null)" = cgroup2fs ] && say "ok    cgroup v2 (unified hierarchy)" || bad "cgroup v2 unified hierarchy required at /sys/fs/cgroup"
if command -v systemctl >/dev/null 2>&1; then
  sdv=$(systemctl --version | awk 'NR==1{print $2}')
  if [ "${sdv:-0}" -ge 257 ] 2>/dev/null; then say "ok    systemd $sdv (>= 257: ProtectControlGroups=private)"; else bad "systemd ${sdv:-?} is older than 257 (launcher unit uses ProtectControlGroups=private, DelegateSubgroup=)"; fi
else bad "systemd not found"; fi
for t in curl sha256sum tar readelf useradd groupadd getent install udevadm systemd-tmpfiles; do
  command -v "$t" >/dev/null 2>&1 || bad "missing tool: $t"
done
# Mitigations and SMT: report, never change.
smt=$(cat /sys/devices/system/cpu/smt/control 2>/dev/null || echo unknown)
case $smt in
  on) warn "SMT is on: a VM's sibling hyperthread may run another tenant. The guests use smt=false in the VM config, but the host policy is yours; see the doc" ;;
  *) say "ok    SMT control: $smt" ;;
esac
if grep -qw 'mitigations=off' /proc/cmdline 2>/dev/null; then bad "kernel command line has mitigations=off"; fi
for v in meltdown spectre_v1 spectre_v2 l1tf mds mmio_stale_data retbleed srbds; do
  f=/sys/devices/system/cpu/vulnerabilities/$v
  [ -r "$f" ] && printf 'info  %-16s %s\n' "$v" "$(cat "$f")"
done
# The jail base must not be mounted nodev: the jailer mknod()s /dev/kvm inside each chroot.
if command -v findmnt >/dev/null 2>&1; then
  opts=$(findmnt -no OPTIONS -T "$(dirname "$STATE")" 2>/dev/null || true)
  case ",$opts," in *,nodev,*) bad "$(dirname "$STATE") is on a nodev mount; the jail base needs device nodes" ;; esac
fi
# The VM uid range must be unused.
if getent passwd | awk -F: -v lo=$VM_UID_MIN -v hi=$VM_UID_MAX '$3>=lo && $3<=hi {found=1} END{exit !found}'; then
  bad "an account uses a uid in the reserved VM range $VM_UID_MIN-$VM_UID_MAX"
else say "ok    uids $VM_UID_MIN-$VM_UID_MAX unused"; fi

if [ "$problems" != 0 ]; then
  if [ "$dry" = 1 ]; then say "(dry run: $problems check(s) would stop a real install)"; else die "$problems check(s) failed; nothing was changed"; fi
fi

# ---- 2. Firecracker (static release binaries, checksum-verified) -----------------------------------------------
say "== firecracker $FC_VERSION"
target=$LIBEXEC/$FC_VERSION
if [ -x "$target/firecracker" ] && [ -x "$target/jailer" ] && [ "$("$target/firecracker" --version 2>/dev/null | head -n 1)" = "Firecracker $FC_VERSION" ]; then
  say "ok    already installed in $target"
elif [ "$dry" = 1 ]; then
  say "DRY   download $FC_URL (or use --firecracker-tarball), require sha256 $FC_TARBALL_SHA256"
  say "DRY   verify the SHA256SUMS inside the archive, require both binaries to be static (no PT_INTERP)"
  say "DRY   install -m 0755 firecracker jailer into $target (root:root)"
else
  tmp=$(mktemp -d)
  trap 'rm -rf "$tmp"' EXIT
  if [ -z "$tarball" ]; then
    curl --proto '=https' --tlsv1.2 -fsSL -o "$tmp/$FC_TARBALL_NAME" "$FC_URL"
    tarball=$tmp/$FC_TARBALL_NAME
  fi
  echo "$FC_TARBALL_SHA256  $tarball" | sha256sum -c - >/dev/null 2>&1 || die "sha256 of $tarball does not match the pinned $FC_TARBALL_SHA256"
  mkdir "$tmp/x"
  tar -C "$tmp/x" -xzf "$tarball"
  rel=$tmp/x/release-$FC_VERSION-$FC_ARCH
  [ -d "$rel" ] || die "unexpected archive layout"
  (cd "$rel" && sha256sum -c --ignore-missing SHA256SUMS >/dev/null 2>&1) || die "inner SHA256SUMS check failed"
  for b in firecracker jailer; do
    src=$rel/$b-$FC_VERSION-$FC_ARCH
    [ -f "$src" ] || die "missing $src"
    # A dynamically linked binary cannot exec inside the empty jailer chroot ("No such file or directory").
    if readelf -lW "$src" | grep -q INTERP; then die "$b is dynamically linked"; fi
  done
  install -d -m 0755 -o root -g root "$LIBEXEC" "$target"
  install -m 0755 -o root -g root "$rel/firecracker-$FC_VERSION-$FC_ARCH" "$target/firecracker"
  install -m 0755 -o root -g root "$rel/jailer-$FC_VERSION-$FC_ARCH" "$target/jailer"
  say "installed $target/firecracker and jailer"
fi
# `current` is what the units point at. The jailer names its chroot and cgroup after the exec file's basename, so keep it "firecracker".
run ln -sfn "$target" "$LIBEXEC/current.new"
run mv -Tf "$LIBEXEC/current.new" "$LIBEXEC/current"

# ---- 3. Users, groups, directories -----------------------------------------------------------------------------------
say "== users, groups, directories"
for g in "$G_CLIENT" "$G_LAUNCH"; do
  if getent group "$g" >/dev/null; then say "ok    group $g"; else run groupadd --system "$g"; fi
done
if getent passwd "$U_BROKER" >/dev/null; then say "ok    user $U_BROKER"
else
  nologin=$(command -v nologin || echo /usr/sbin/nologin)
  # Primary group is the web-facing socket group; the launcher group is a supplementary one set in the unit.
  run useradd --system --no-create-home --home-dir / --shell "$nologin" --gid "$G_CLIENT" --groups "$G_LAUNCH" "$U_BROKER"
fi
run install -d -m 0755 -o root -g root "$LIBEXEC" "$SHARE" "$SHARE/images"
run install -d -m 0700 -o root -g root "$STATE" "$STATE/jail"
run install -d -m 0750 -o root -g "$U_BROKER" "$ETC"   # optional launcher.env / broker.env; no secrets belong here
say "NOTE  the web service must join group $G_CLIENT (SupplementaryGroups=) to reach the broker socket"

# ---- 4. systemd, tmpfiles, udev ----------------------------------------------------------------------------------
say "== units, tmpfiles.d, udev rule"
for u in baremetal-shell.slice baremetal-shell-launcher.service baremetal-shell-broker.service; do
  run install -m 0644 -o root -g root "$repo/deploy/systemd/$u" "/etc/systemd/system/$u"
done
run install -m 0644 -o root -g root "$repo/deploy/shell/tmpfiles.d/baremetal-shell.conf" /etc/tmpfiles.d/baremetal-shell.conf
run install -m 0644 -o root -g root "$repo/deploy/shell/udev/99-baremetal-kvm.rules" /etc/udev/rules.d/99-baremetal-kvm.rules
run systemd-tmpfiles --create /etc/tmpfiles.d/baremetal-shell.conf
run udevadm control --reload
run udevadm trigger --action=change --subsystem-match=misc --sysname-match=kvm
run systemctl daemon-reload

# ---- 5. What is still missing ---------------------------------------------------------------------------------------
say "== remaining manual steps"
[ -f "$SHARE/images/vmlinux" ] || say "TODO  build and install the guest kernel: scripts/shell/build-kernel.sh, then install -m 0644 vmlinux $SHARE/images/vmlinux"
[ -f "$SHARE/images/rootfs.ext4" ] || say "TODO  build the rootfs: scripts/shell/build-rootfs.sh out.ext4, then install -m 0644 out.ext4 $SHARE/images/rootfs.ext4"
say "TODO  load deploy/shell/nftables/host-egress.nft after replacing its TODO addresses"
say "TODO  copy the repository to /opt/baremetal-shell (root-owned, read-only) and run npm ci --omit=dev there"
say "TODO  work through docs/shell-deployment.md (pre-launch checklist) before 'systemctl enable --now baremetal-shell-launcher baremetal-shell-broker'"
if [ "$dry" = 1 ]; then say "dry run complete: nothing was changed"; fi
exit 0
