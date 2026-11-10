---
title: A RAM Disk with Fixed Blocks
summary: Read and write bounded byte ranges on a memory-backed block device.
minutes: 40
difficulty: medium
tags: kernel, systems
id: osdev-2/ramdisk
mode: harness
---
Before a kernel can have files it needs somewhere to keep them. Real storage (an AHCI disk, an NVMe drive, a virtio device) needs a driver with queues, interrupts and DMA. A **RAM disk** skips all of that: it is an array of bytes in memory that *pretends* to be a storage device. Kernels use one as the initial root filesystem (Linux's `initramfs`), as scratch space, and, here, as a way to build a filesystem layer before writing any disk driver.

## Layers

The point of a device interface is that the layers above it don't know what is underneath:

```text
   file_read(fd, buf, n)            files: descriptors, offsets   (lesson 12)
            |
   fs_read(inode, off, buf, n)      filesystem: names, directories, block maps
            |
   disk_read(off, buf, len)         device: bytes at offsets      (this lesson)
            |
   RAM array  /  AHCI driver  /  virtio-blk
```

The device knows nothing about files, names or directories. It knows its size, and it copies bytes in and out at offsets. A filesystem written against `disk_read` and `disk_write` keeps working when the RAM array is swapped for a real driver. That is also why the device must be strict: it is the last line of defence before a filesystem bug becomes corrupted storage.

(Real block devices transfer whole **sectors**, usually 512 bytes, and the filesystem rounds every request out to sector boundaries. This device accepts any byte range to keep the focus on the bounds checks, which are the same either way.)

## Validate the whole request first

A request is an offset and a length. It is valid when every byte it touches lies inside the disk, which is the half-open range `[0, 512)`:

```text
offset              offset + len
  |<------ len ------->|
  v                    v
[ 0 ......................................... 512 )
```

Two failure modes must be prevented.

**Partial writes.** Suppose a write starts in range but runs off the end. A loop that checks each byte as it goes would copy the first few bytes and then fail. The caller gets an error, but the disk has changed anyway. A filesystem that retries or reports the error now has a disk whose contents match neither the old state nor the new one. The rule for every device and every filesystem operation is: **check everything, then do everything**. A failed request leaves the disk exactly as it was, and a failed read leaves the caller's buffer untouched.

**Overflowing the check.** The obvious test is

```c
if (offset + len > 512) return -1;      /* WRONG with unsigned numbers */
```

`size_t` arithmetic wraps around modulo 2^64. A caller (or an attacker controlling a system call argument) can pick values whose sum wraps to something small:

```text
offset = 10, len = 0xFFFFFFFFFFFFFFFB  (that is (size_t)-5)
offset + len = 0x10000000000000005 -> wraps to 5
5 > 512 is false -> the request is "valid" -> copies ~2^64 bytes
```

The fix is the same "subtract instead of add" you used for frame reservations. First check the offset alone, then compare the length with the space that is left after it:

```c
if (offset > 512) return -1;            /* now 512 - offset cannot wrap */
if (len > 512 - offset) return -1;      /* no addition anywhere */
```

Notice the first comparison is `>`, not `>=`. An offset of exactly 512 is the end of the disk, and a zero-length request there is valid; it transfers nothing. That edge case shows up whenever a caller reads "everything that is left": at the end of the disk, everything that is left is nothing.

## Worked examples

```text
request                  offset  len   result
write 6 bytes at 123      123     6    ok: 123 + 6 = 129 <= 512
write 1 byte at 511       511     1    ok: the last byte
read 0 bytes at 512       512     0    ok: empty, at the end
write 6 bytes at 510      510     6    -1: 6 > 512 - 510 = 2   (nothing is written)
read 1 byte at 512        512     1    -1: 1 > 0
write 0 bytes at 513      513     0    -1: offset is past the end
read at (size_t)-2        huge    6    -1: offset > 512
read 10, (size_t)-5        10   huge   -1: len > 502
```

## Initialisation

`disk_init` zeroes all 512 bytes, so a fresh disk reads as zeros. Formatting a filesystem on top would write its metadata over those zeros; a filesystem that finds an all-zero superblock knows the disk is blank.

## Copying

Once a request is validated, the copy is a simple byte loop between the disk array and the caller's buffer. The buffers are `void *` so the caller can pass any type; convert to `uint8_t *` to index bytes. In this exercise the caller's buffer never overlaps the disk array, so the copy direction does not matter.

## Your turn

The 512-byte `disk` array is in the starter. Implement `disk_init`, `disk_read` and `disk_write` so that every invalid request fails without side effects. The tests try the boundaries above and check that failed writes change nothing.
