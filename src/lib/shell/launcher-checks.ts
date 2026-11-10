/**
 * Start-up checks for the root launcher (scripts/shell-launcher.mts), kept free of I/O so they can be tested with
 * made-up stat results. Each returns a problem description, or null when the directory is acceptable.
 */
export type DirStat = { uid: number; gid: number; mode: number };

/**
 * The launcher socket's directory decides who can reach or replace the socket. Jailed, it must be root-owned, not
 * writable by group or others, and its group must be SHELL_BROKER_GID: the launcher hands that gid every VM's vsock
 * socket and the launcher socket itself, so a mistyped number would give them to some other group. The directory's
 * group is set by tmpfiles.d (baremetal-shell-launch), so it pins the number to the right group.
 */
export function launcherSocketDirProblem(st: DirStat, options: { jailed: boolean; brokerGid?: number }): string | null {
  if (st.mode & 0o002 && !(st.mode & 0o1000)) return "SHELL_LAUNCHER_SOCKET must not be in a world-writable directory";
  if (!options.jailed) return null;
  if (st.uid !== 0) return "SHELL_LAUNCHER_SOCKET's directory must be owned by root";
  if (st.mode & 0o022) return "SHELL_LAUNCHER_SOCKET's directory must not be writable by group or others";
  if (options.brokerGid === undefined || st.gid !== options.brokerGid) {
    return `SHELL_BROKER_GID (${options.brokerGid ?? "unset"}) must equal the group of SHELL_LAUNCHER_SOCKET's directory (${st.gid}); set it to the gid of baremetal-shell-launch`;
  }
  return null;
}

/** The broker reaches vsock sockets through <chrootBase>/sockets, so it must be able to search chrootBase. */
export function chrootBaseProblem(st: DirStat, brokerGid: number): string | null {
  if (st.mode & 0o001 || (st.gid === brokerGid && st.mode & 0o010)) return null;
  return "SHELL_CHROOT_BASE must be searchable by the broker (mode 0711, or group SHELL_BROKER_GID with g+x)";
}
