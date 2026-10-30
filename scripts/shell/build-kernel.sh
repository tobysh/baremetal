#!/bin/sh
# Build the pinned, minimal guest kernel for the interactive shell microVMs.
#
#   build-kernel.sh [--out DIR] [--work DIR] [--jobs N] [--tarball FILE] [--config-only]
#
# Base: Firecracker's x86_64 6.1 microvm config (vendored in deploy/shell/kernel/, hash-checked), reduced here.
# Reproducibility: the tarball and base config are pinned by sha256; the build uses a fixed timestamp, user, host and
# locale. The output is bit-identical only for the same compiler, binutils and absolute work directory, so the
# MANIFEST records the toolchain. Build on a machine that is not the shell host; copy only vmlinux to the host.
#
# --config-only stops after the configuration has been resolved and asserted (about a minute, no compile).
# A full build takes tens of minutes and has NOT been run for this repository version; see docs/shell-deployment.md.
set -eu

KERNEL_VERSION=6.1.191
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${KERNEL_VERSION}.tar.xz"
# From https://cdn.kernel.org/pub/linux/kernel/v6.x/sha256sums.asc, whose PGP signature (kernel.org autosigner key
# B8868C80BA62A1FFFAF5FDA9632D3A06589DA6B1) was checked on 30 October 2026. Re-check both when changing the version.
KERNEL_SHA256=02f15b77d213e15607923832ef9382e2414ea90df6c007f15a910e139645c35d
BASE_CONFIG_SHA256=153ca1b40f3312bfb40b7587b471ea548a915f98f480f0d0892a1537957a2147

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
base_config=$repo/deploy/shell/kernel/firecracker-microvm-x86_64-6.1.config
out=$repo/.data/shell-kernel
work=
jobs=$(nproc 2>/dev/null || echo 2)
tarball=
config_only=0

while [ $# -gt 0 ]; do
  case $1 in
    --out) out=${2:?--out needs a directory}; shift 2 ;;
    --work) work=${2:?--work needs a directory}; shift 2 ;;
    --jobs) jobs=${2:?--jobs needs a number}; shift 2 ;;
    --tarball) tarball=${2:?--tarball needs a file}; shift 2 ;;
    --config-only) config_only=1; shift ;;
    -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

die() { echo "build-kernel: $*" >&2; exit 1; }
need() { command -v "$1" >/dev/null 2>&1 || die "missing tool: $1"; }
for t in tar sha256sum make gcc flex bison awk grep sed; do need "$t"; done
[ "$(uname -m)" = x86_64 ] || die "this script builds the x86_64 guest kernel on an x86_64 machine"
[ "$config_only" = 1 ] || { need bc; need ld; need perl; need xz; }

check_sha() { # file expected
  echo "$2  $1" | sha256sum -c - >/dev/null 2>&1 || die "sha256 mismatch for $1 (expected $2)"
}

[ -f "$base_config" ] || die "missing $base_config"
check_sha "$base_config" "$BASE_CONFIG_SHA256"

mkdir -p "$out"
[ -n "$work" ] || work=$(mktemp -d)
mkdir -p "$work"
work=$(cd "$work" && pwd)
[ -z "$(ls -A "$work")" ] || die "work directory $work must be empty"

if [ -z "$tarball" ]; then
  need curl
  tarball=$work/linux-${KERNEL_VERSION}.tar.xz
  curl --proto '=https' --tlsv1.2 -fsSL -o "$tarball" "$KERNEL_URL"
fi
check_sha "$tarball" "$KERNEL_SHA256"   # verified BEFORE extraction

tar -C "$work" -xf "$tarball"
src=$work/linux-${KERNEL_VERSION}
[ -d "$src" ] || die "tarball did not contain linux-${KERNEL_VERSION}/"

# Deterministic environment.
export LC_ALL=C TZ=UTC
export KBUILD_BUILD_USER=baremetal KBUILD_BUILD_HOST=build KBUILD_BUILD_VERSION=1
export KBUILD_BUILD_TIMESTAMP='Thu Jan  1 00:00:00 UTC 1970'
export SOURCE_DATE_EPOCH=0

cd "$src"
cp "$base_config" .config

cfg() { ./scripts/config --file .config "$@"; }

# EXPERT makes options such as IO_URING visible; while it is off, an invisible symbol silently falls back to its default (y).
cfg -e EXPERT

# --- Reductions on top of Firecracker's config -------------------------------------------------------------------
# No loadable modules is already the base's setting; it is asserted below rather than changed here.
# Attack surface that a learner shell does not need (each of these has had local-privilege-escalation bugs):
for sym in USER_NS IO_URING BPF_SYSCALL KEXEC KEXEC_FILE CRASH_DUMP DEBUG_FS USERFAULTFD KSM \
           SYSFS_SYSCALL MAGIC_SYSRQ BINFMT_MISC FUSE_FS OVERLAY_FS PROC_KCORE DEVPORT DEVMEM \
           X86_MSR X86_CPUID MODIFY_LDT_SYSCALL IA32_EMULATION X86_X32_ABI X86_VSYSCALL_EMULATION \
           X86_IOPL_IOPERM PROFILING AUDIT CHECKPOINT_RESTORE POSIX_MQUEUE SYSVIPC \
           HIBERNATION SECURITYFS LEGACY_TIOCSTI; do
  cfg -d "$sym"
done
# PERF_EVENTS cannot be disabled on x86 in 6.1 (arch/x86/Kconfig selects it); the default kernel.perf_event_paranoid=2
# keeps it away from unprivileged users, and the guest has no perf tooling. Do not "fix" this by editing the assertions.
# Network: keep CONFIG_NET + INET (AF_UNIX, AF_VSOCK, loopback and the virtio-net NIC that a few labs get) but drop the
# rest of the stack. Labs that need packet sockets or netfilter inside the guest need a second kernel build.
for sym in IPV6 NETFILTER XFRM PACKET; do cfg -d "$sym"; done
# Hardening that is cheap for a 256 MiB guest.
for sym in SECURITY_DMESG_RESTRICT INIT_ON_ALLOC_DEFAULT_ON SLAB_FREELIST_HARDENED SLAB_FREELIST_RANDOM \
           RANDOMIZE_BASE RANDOMIZE_MEMORY HARDENED_USERCOPY FORTIFY_SOURCE STACKPROTECTOR_STRONG \
           SCHED_STACK_END_CHECK BUG_ON_DATA_CORRUPTION; do
  cfg -e "$sym"
done
# What the guest agent and /init need, stated explicitly so a base-config change cannot silently remove it.
for sym in VSOCKETS VIRTIO_VSOCKETS VIRTIO_BLK VIRTIO_MMIO VIRTIO_MMIO_CMDLINE_DEVICES EXT4_FS DEVTMPFS DEVTMPFS_MOUNT \
           TMPFS PROC_FS SYSFS UNIX98_PTYS UNIX SERIAL_8250 SERIAL_8250_CONSOLE TTY BLK_DEV; do
  cfg -e "$sym"
done

make ARCH=x86_64 olddefconfig >/dev/null

# --- Assertions: the resolved config, not the intent, is what ships ----------------------------------------------
fail=0
want() { grep -qx "CONFIG_$1=y" .config || { echo "build-kernel: CONFIG_$1 must be =y" >&2; fail=1; }; }
unwanted() {
  if grep -Eq "^CONFIG_$1=[ym]" .config; then echo "build-kernel: CONFIG_$1 must be off" >&2; fail=1; fi
}
for sym in VSOCKETS VIRTIO_VSOCKETS VIRTIO_BLK VIRTIO_MMIO EXT4_FS DEVTMPFS TMPFS PROC_FS SYSFS UNIX98_PTYS UNIX \
           SERIAL_8250_CONSOLE NET INET SECCOMP; do want "$sym"; done
for sym in MODULES USER_NS IO_URING BPF_SYSCALL KEXEC KEXEC_FILE DEBUG_FS USERFAULTFD MAGIC_SYSRQ BINFMT_MISC FUSE_FS \
           OVERLAY_FS PROC_KCORE DEVMEM DEVPORT X86_MSR IA32_EMULATION IPV6 NETFILTER PACKET AUDIT \
           SYSVIPC HIBERNATION KPROBES FTRACE DEBUG_INFO; do unwanted "$sym"; done
[ "$fail" = 0 ] || die "resolved configuration violates the assertions above (a dependency re-enabled something?)"

cp .config "$out/config.used"
echo "build-kernel: configuration for linux-${KERNEL_VERSION} resolved and verified -> $out/config.used"
[ "$config_only" = 0 ] || exit 0

# --- Build ---------------------------------------------------------------------------------------------------------
make ARCH=x86_64 -j"$jobs" vmlinux
cp vmlinux "$out/vmlinux"
(cd "$out" && sha256sum vmlinux config.used > SHA256SUMS)
{
  echo "kernel $KERNEL_VERSION"
  echo "tarball_sha256 $KERNEL_SHA256"
  echo "base_config_sha256 $BASE_CONFIG_SHA256"
  echo "gcc $(gcc --version | head -n 1)"
  echo "ld $(ld --version | head -n 1)"
  echo "workdir $work"
} > "$out/MANIFEST"
echo "build-kernel: wrote $out/vmlinux"
cat "$out/SHA256SUMS"
echo "Install on the shell host as root: install -m 0644 -o root -g root vmlinux /usr/local/share/baremetal-shell/images/vmlinux"
