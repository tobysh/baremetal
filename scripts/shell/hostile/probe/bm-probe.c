// Guest-side permission and containment probe for the hostile-workload suite. BENIGN: every call is made as the
// unprivileged learner and is EXPECTED to be refused. It exploits nothing; if a call unexpectedly succeeds it
// reports FAIL and does nothing further with the result. Output uses the suite's BM_CHECK line format.
// Build (on the disposable host, never in the guest): gcc -static -O2 -Wall -o bm-probe bm-probe.c
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/io.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/vm_sockets.h>

static void report(const char *name, const char *verdict, const char *fmt, int err) {
  printf("BM_CHECK\t%s\t%s\t", name, verdict);
  printf(fmt, err, strerror(err));
  printf("\n");
}
// Denied means the call failed with a permission-type errno. Anything else (success, EINVAL...) is reported so a human looks.
static void denied(const char *name, long rc) {
  int e = errno;
  if (rc == -1 && (e == EPERM || e == EACCES || e == ENOSYS || e == ENOENT)) report(name, "PASS", "refused: errno %d (%s)", e);
  else if (rc == -1) report(name, "INFO", "failed with unexpected errno %d (%s): review", e);
  else printf("BM_CHECK\t%s\tFAIL\tcall SUCCEEDED (rc=%ld): the operation was permitted\n", name, rc);
}

int main(void) {
  printf("BM_CHECK\tprobe.ids\tINFO\tuid=%d euid=%d gid=%d\n", getuid(), geteuid(), getgid());
  denied("probe.setuid0", setuid(0));
  denied("probe.setgid0", setgid(0));
  denied("probe.unshare-newns", unshare(CLONE_NEWNS));
  denied("probe.unshare-newpid", unshare(CLONE_NEWPID));
  denied("probe.unshare-newnet", unshare(CLONE_NEWNET));
  denied("probe.chroot", chroot("/tmp"));
  denied("probe.mount-tmpfs", mount("tmpfs", "/mnt", "tmpfs", 0, NULL));
  denied("probe.umount-proc", umount("/proc"));
  denied("probe.pivot-root", syscall(SYS_pivot_root, "/tmp", "/tmp"));
  int ns = open("/proc/1/ns/mnt", O_RDONLY);
  if (ns < 0) report("probe.setns-pid1-mnt", "PASS", "cannot even open /proc/1/ns/mnt: errno %d (%s)", errno);
  else denied("probe.setns-pid1-mnt", setns(ns, CLONE_NEWNS));
  denied("probe.ptrace-attach-1", ptrace(PTRACE_ATTACH, 1, 0, 0));
  denied("probe.ptrace-seize-1", ptrace(PTRACE_SEIZE, 1, 0, 0));
  denied("probe.mknod-chr-mem", mknod("/tmp/bm-mem", S_IFCHR | 0600, makedev(1, 1)));
  denied("probe.mknod-blk", mknod("/tmp/bm-blk", S_IFBLK | 0600, makedev(254, 0)));
  denied("probe.reboot", reboot(RB_DISABLE_CAD));
  denied("probe.kexec_load", syscall(SYS_kexec_load, 0, 0, NULL, 0));
  denied("probe.init_module", syscall(SYS_init_module, NULL, 0, ""));
  denied("probe.finit_module", syscall(SYS_finit_module, -1, "", 0));
  denied("probe.delete_module", syscall(SYS_delete_module, "bm", 0));
  denied("probe.iopl", iopl(3));
  denied("probe.ioperm", ioperm(0, 0x400, 1));
  struct timespec ts = { 0, 0 };
  denied("probe.clock_settime", clock_settime(CLOCK_REALTIME, &ts));
  denied("probe.sethostname", sethostname("pwned", 5));
  denied("probe.swapon", syscall(SYS_swapon, "/tmp/x", 0));
  struct rlimit rl = { 100000, 100000 };
  denied("probe.raise-nproc-hard", setrlimit(RLIMIT_NPROC, &rl));
  denied("probe.raise-nofile-hard", setrlimit(RLIMIT_NOFILE, &(struct rlimit){ 100000000, 100000000 }));
  denied("probe.open-kvm", open("/dev/kvm", O_RDWR));
  denied("probe.open-mem", open("/dev/mem", O_RDONLY));
  denied("probe.open-port", open("/dev/port", O_RDONLY));
  denied("probe.open-kmsg", open("/dev/kmsg", O_RDONLY));
  denied("probe.raw-socket", socket(AF_INET, SOCK_RAW, IPPROTO_RAW));
  denied("probe.packet-socket", socket(AF_PACKET, SOCK_RAW, 0));
  // vsock toward the HOST (CID 2): Firecracker maps guest-initiated connections to host-side unix sockets
  // "<uds_path>_<port>"; none must exist, so every connect must fail. Success means the VMM exposes a host listener.
  int open_ports = 0, tried = 0;
  int ports[] = { 1, 22, 80, 443, 1024, 3000, 5000, 5001, 8080, 9999, 27017, 49152, 65535 };
  for (unsigned i = 0; i < sizeof ports / sizeof *ports; i++) {
    int s = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (s < 0) { report("probe.vsock-socket", "INFO", "AF_VSOCK socket failed: errno %d (%s)", errno); break; }
    struct sockaddr_vm a = { .svm_family = AF_VSOCK, .svm_cid = VMADDR_CID_HOST, .svm_port = (unsigned)ports[i] };
    struct timeval tv = { 2, 0 };
    setsockopt(s, AF_VSOCK, SO_VM_SOCKETS_CONNECT_TIMEOUT, &tv, sizeof tv);
    tried++;
    if (connect(s, (struct sockaddr *)&a, sizeof a) == 0) { open_ports++; printf("BM_CHECK\tprobe.vsock-host-port-%d\tFAIL\tCONNECTED to host CID 2\n", ports[i]); }
    close(s);
  }
  if (tried && !open_ports) printf("BM_CHECK\tprobe.vsock-to-host\tPASS\tall %d connects to CID 2 failed\n", tried);
  // User namespaces: allowed by default on many kernels, so this is informational. If it works, the guest's own
  // root-in-userns surface is reachable; that is inside the guest and not a host escape, but it is recorded.
  int r = unshare(CLONE_NEWUSER);
  if (r == 0) printf("BM_CHECK\tprobe.userns\tINFO\tunprivileged user namespaces are ENABLED in the guest kernel (consider kernel.unprivileged_userns_clone=0 or max_user_namespaces=0)\n");
  else report("probe.userns", "PASS", "unshare(CLONE_NEWUSER) refused: errno %d (%s)", errno);
  return 0;
}
