// Restore a backup made by backup-db.mts.
//   npm run restore-db -- <backup-dir> [--force] [--db <name>]
//   npm run restore-db -- <backup-dir> --verify
// Refuses to restore into a database that already contains documents unless --force
// is given (--force drops the collections present in the backup first). Stop the site first.
// --verify restores into a throwaway database, compares per-collection counts, drops it.
import path from "node:path";
import type { MongoClient } from "mongodb";
import { compareCounts, connect, countAll, dbName, loadInto, readManifest, verifyBackup } from "./lib/backup.mjs";

const args = process.argv.slice(2);
const di = args.indexOf("--db");
const target = di >= 0 ? args[di + 1] : dbName();
const dir = args.find((a, i) => !a.startsWith("--") && args[i - 1] !== "--db");
if (!dir || !target) {
  console.error("Usage: restore-db <backup-dir> [--force] [--db <name>] | restore-db <backup-dir> --verify");
  process.exit(2);
}
const src = path.resolve(dir);

let client: MongoClient | undefined;
try {
  const manifest = await readManifest(src);
  client = await connect();
  if (args.includes("--verify")) {
    const { problems, table } = await verifyBackup(client, src);
    console.log(table.join("\n"));
    if (problems.length) {
      console.error("VERIFY FAILED:\n  " + problems.join("\n  "));
      process.exitCode = 1;
    } else console.log("Verify OK: counts match (throwaway database dropped).");
  } else {
    const db = client.db(target);
    const existing = await countAll(db);
    const nonEmpty = Object.entries(existing).filter(([, n]) => n > 0);
    if (nonEmpty.length && !args.includes("--force")) {
      console.error(`Refusing to restore: database "${target}" already has data (${nonEmpty.map(([c, n]) => `${c}=${n}`).join(", ")}). Re-run with --force to replace the backed-up collections.`);
      process.exitCode = 3;
    } else {
      for (const c of Object.keys(manifest.collections)) if (c in existing) await db.collection(c).drop();
      await loadInto(db, src, manifest);
      const expected = Object.fromEntries(Object.entries(manifest.collections).map(([c, i]) => [c, i.count]));
      const problems = compareCounts(expected, await countAll(db));
      if (problems.length) {
        console.error("Restore finished but counts differ:\n  " + problems.join("\n  "));
        process.exitCode = 1;
      } else console.log(`Restored ${Object.keys(expected).length} collections into "${target}"; counts match.`);
    }
  }
} catch (e) {
  console.error((e as Error).message);
  process.exitCode = 1;
} finally {
  await client?.close();
}
