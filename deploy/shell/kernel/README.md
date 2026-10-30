# Guest kernel base configuration

`firecracker-microvm-x86_64-6.1.config` is `resources/guest_configs/microvm-kernel-ci-x86_64-6.1.config` from the
Firecracker repository at tag `v1.17.0`, unmodified (sha256 `153ca1b40f3312bfb40b7587b471ea548a915f98f480f0d0892a1537957a2147`).
`scripts/shell/build-kernel.sh` checks that hash, applies its own reductions with `scripts/config`, runs `make olddefconfig`
and then asserts the result. When bumping Firecracker, replace this file from the new tag, update the hash in the script
and re-read the assertions. Upstream's disclaimer applies: these configs are tuned for short-lived, high-density guests.
