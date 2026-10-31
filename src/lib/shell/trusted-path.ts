import { stat } from "node:fs/promises";
import path from "node:path";

/**
 * The root launcher executes the jailer and Firecracker and copies the kernel and rootfs, so the file alone being
 * root-owned is not enough: anyone who can write to a parent directory can rename a different file into place and
 * have it run as root. Every path component, from the resolved file up to `/`, must be owned by root and not
 * writable by group or others. A sticky directory (like /tmp) is tolerated for components it holds, since entries
 * in it cannot be replaced by users who do not own them; those components still have to be root-owned themselves.
 * `resolved` must already be a realpath, so no symlink can redirect the walk.
 */
export async function untrustedComponent(resolved: string): Promise<string | null> {
  for (let current = resolved; ; current = path.dirname(current)) {
    const st = await stat(current);
    if (st.uid !== 0) return `${current} is not owned by root`;
    const isRoot = current === resolved;
    if (st.mode & 0o022) {
      // Only a sticky directory may be group/other-writable, and never the file or directory being trusted itself.
      if (isRoot || !st.isDirectory() || !(st.mode & 0o1000)) return `${current} is writable by group or others`;
    }
    if (current === path.parse(current).root) return null;
  }
}
