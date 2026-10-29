// Back up the app database to gzipped Extended-JSON lines (pure Node, no mongodump needed).
//   npm run backup-db [-- --verify] [-- --keep N]
// Env: MONGODB_URI, MONGODB_DB (same defaults as the app), BACKUP_DIR (default ~/baremetal-backups),
//      BACKUP_KEEP (default 14, newest backups kept).
// The backup holds password hashes and TOTP secrets: see docs/deployment-hardening.md.
import path from "node:path";
import type { MongoClient } from "mongodb";
import { backupRoot, connect, dbName, dumpDb, prune, verifyBackup } from "./lib/backup.mjs";

const args = process.argv.slice(2);
const verify = args.includes("--verify");
const ki = args.indexOf("--keep");
const keep = Number(ki >= 0 ? args[ki + 1] : process.env.BACKUP_KEEP ?? 14);
if (!Number.isInteger(keep) || keep < 1) {
  console.error("--keep / BACKUP_KEEP must be a positive integer");
  process.exit(2);
}

const name = dbName();
const repo = path.resolve(import.meta.dirname, "..");
if (backupRoot() === repo || backupRoot().startsWith(repo + path.sep)) {
  console.error("Refusing to write backups inside the repository. Set BACKUP_DIR elsewhere.");
  process.exit(2);
}

let client: MongoClient | undefined;
try {
  client = await connect();
  const { dir, manifest } = await dumpDb(client.db(name), name);
  const total = Object.values(manifest.collections).reduce((s, c) => s + c.count, 0);
  console.log(`Backed up ${name}: ${Object.keys(manifest.collections).length} collections, ${total} documents -> ${dir}`);
  if (verify) {
    const { problems, table } = await verifyBackup(client, dir);
    console.log(table.join("\n"));
    if (problems.length) {
      console.error("VERIFY FAILED:\n  " + problems.join("\n  "));
      process.exitCode = 1;
    } else console.log("Verify OK: throwaway restore matches the backup counts (throwaway database dropped).");
  }
  if (!process.exitCode) {
    const removed = await prune(name, keep);
    console.log(`Retention: keeping newest ${keep}; pruned ${removed.length}.`);
  }
} catch (e) {
  console.error((e as Error).message);
  process.exitCode = 1;
} finally {
  await client?.close();
}
