// Guest agent: runs as root inside the microVM, started by /init. It accepts ONE vsock connection from the
// broker, starts an unprivileged login shell on a PTY and relays framed bytes. Everything the broker sends is
// untrusted-in-reverse too: the guest is hostile, so the broker (not this agent) enforces every limit.
//
// Frame: [type u8][len u32 big-endian][payload], payload <= 16 KiB. Types: 0 DATA, 1 RESIZE (u16 cols, u16 rows),
// 2 READY (agent -> broker, sent once the shell is running), 3 EXIT (agent -> broker, i32 status).
//
// Build: gcc -static -O2 -Wall -o agent agent.c
#define _GNU_SOURCE
#include <errno.h>
#include <grp.h>
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <linux/vm_sockets.h>

#define PORT 5000
#define MAX_PAYLOAD 16384
#define LEARNER_UID 1000

enum { T_DATA = 0, T_RESIZE = 1, T_READY = 2, T_EXIT = 3 };

static int write_all(int fd, const void *buf, size_t n) {
  const char *p = buf;
  while (n) {
    ssize_t w = write(fd, p, n);
    if (w < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) { struct pollfd pf = { fd, POLLOUT, 0 }; poll(&pf, 1, 1000); continue; }
      return -1;
    }
    p += w;
    n -= (size_t)w;
  }
  return 0;
}

static int send_frame(int fd, uint8_t type, const void *payload, uint32_t len) {
  uint8_t head[5] = { type, (uint8_t)(len >> 24), (uint8_t)(len >> 16), (uint8_t)(len >> 8), (uint8_t)len };
  if (write_all(fd, head, 5) < 0) return -1;
  return len ? write_all(fd, payload, len) : 0;
}

static void child_shell(int slave) {
  setsid();
  ioctl(slave, TIOCSCTTY, 0);
  dup2(slave, 0); dup2(slave, 1); dup2(slave, 2);
  if (slave > 2) close(slave);
  // Drop everything before exec: bounding set first (needs CAP_SETPCAP), then ids, then no_new_privs.
  for (int cap = 0; cap <= 63; cap++) prctl(PR_CAPBSET_DROP, cap, 0, 0, 0);
  struct rlimit nproc = { 256, 256 };
  setrlimit(RLIMIT_NPROC, &nproc);
  if (setgroups(0, NULL) < 0 || setgid(LEARNER_UID) < 0 || setuid(LEARNER_UID) < 0) _exit(120);
  if (setuid(0) == 0) _exit(121); // must not be able to regain root
  prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
  char *envp[] = { "HOME=/home/learner", "PATH=/bin", "TERM=xterm-256color", "LANG=C.UTF-8", "USER=learner", NULL };
  char *argv[] = { "bash", "--noprofile", "--norc", "-i", NULL };
  chdir("/home/learner");
  execve("/bin/bash", argv, envp);
  _exit(127);
}

int main(void) {
  signal(SIGPIPE, SIG_IGN);
  int ls = socket(AF_VSOCK, SOCK_STREAM, 0);
  if (ls < 0) { perror("socket(AF_VSOCK)"); return 1; }
  struct sockaddr_vm addr = { .svm_family = AF_VSOCK, .svm_port = PORT, .svm_cid = VMADDR_CID_ANY };
  if (bind(ls, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(ls, 1) < 0) { perror("bind/listen"); return 1; }
  int conn = accept(ls, NULL, NULL);
  if (conn < 0) { perror("accept"); return 1; }
  close(ls); // one session per VM

  int master, slave;
  struct winsize ws = { 24, 80, 0, 0 };
  if (openpty(&master, &slave, NULL, NULL, &ws) < 0) { perror("openpty"); return 1; }
  pid_t pid = fork();
  if (pid < 0) return 1;
  if (pid == 0) { close(master); close(conn); child_shell(slave); }
  close(slave);
  send_frame(conn, T_READY, NULL, 0);

  uint8_t inbuf[5 + MAX_PAYLOAD + 5];
  size_t have = 0;
  uint8_t out[MAX_PAYLOAD];
  int status = 0, child_done = 0, master_open = 1;
  for (;;) {
    struct pollfd pf[2] = { { conn, POLLIN, 0 }, { master_open ? master : -1, POLLIN, 0 } };
    if (poll(pf, 2, 200) < 0 && errno != EINTR) break;
    if (pf[0].revents & (POLLIN | POLLHUP | POLLERR)) {
      ssize_t r = read(conn, inbuf + have, sizeof inbuf - have);
      if (r <= 0) break; // broker went away: the VM is about to be destroyed anyway
      have += (size_t)r;
      for (;;) {
        if (have < 5) break;
        uint32_t len = ((uint32_t)inbuf[1] << 24) | (inbuf[2] << 16) | (inbuf[3] << 8) | inbuf[4];
        if (len > MAX_PAYLOAD) return 2;
        if (have < 5 + len) break;
        uint8_t type = inbuf[0];
        uint8_t *payload = inbuf + 5;
        if (type == T_DATA && master_open) write_all(master, payload, len);
        else if (type == T_RESIZE && len == 4) {
          struct winsize w = { (unsigned short)((payload[2] << 8) | payload[3]), (unsigned short)((payload[0] << 8) | payload[1]), 0, 0 };
          if (w.ws_col >= 1 && w.ws_col <= 500 && w.ws_row >= 1 && w.ws_row <= 200) ioctl(master, TIOCSWINSZ, &w);
        }
        memmove(inbuf, inbuf + 5 + len, have - 5 - len);
        have -= 5 + len;
      }
    }
    if (master_open && (pf[1].revents & (POLLIN | POLLHUP))) {
      ssize_t r = read(master, out, sizeof out);
      if (r > 0) { if (send_frame(conn, T_DATA, out, (uint32_t)r) < 0) break; }
      else master_open = 0; // EIO once the shell has exited
    }
    if (!child_done && waitpid(pid, &status, WNOHANG) == pid) child_done = 1;
    if (child_done && !master_open) break;
  }
  if (!child_done) { kill(pid, SIGKILL); waitpid(pid, &status, 0); }
  int32_t code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
  uint8_t ex[4] = { (uint8_t)(code >> 24), (uint8_t)(code >> 16), (uint8_t)(code >> 8), (uint8_t)code };
  send_frame(conn, T_EXIT, ex, 4);
  return 0;
}
