// Shared helpers for backup-db.mts and restore-db.mts.
// Format: <BACKUP_DIR>/<db>-<UTC timestamp>/ containing manifest.json plus one
// <collection>.jsonl.gz per collection (one canonical Extended JSON document per
// line, so ObjectId, Date and numeric types survive). Indexes are in the manifest.
import { createReadStream, createWriteStream } from "node:fs";
import { chmod, mkdir, readdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import { homedir } from "node:os";
import path from "node:path";
import { createGunzip, createGzip } from "node:zlib";
import { createInterface } from "node:readline";
import { pipeline } from "node:stream/promises";
import { Readable } from "node:stream";
import { BSON, MongoClient, type Db, type Document } from "mongodb";

const { EJSON } = BSON;

export const FORMAT = 1;
export const uri = () => process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017";
export const dbName = () => process.env.MONGODB_DB ?? "baremetal";
export const backupRoot = () => path.resolve(process.env.BACKUP_DIR ?? path.join(homedir(), "baremetal-backups"));

export type Manifest = {
  format: number;
  db: string;
  createdAt: string;
  collections: Record<string, { count: number; file: string; indexes: Document[] }>;
};

export function redact(s: string): string {
  return s.replace(/\/\/[^@/]*@/, "//***@");
}

export async function connect(): Promise<MongoClient> {
  const client = new MongoClient(uri(), { serverSelectionTimeoutMS: 8000 });
  try {
    await client.connect();
    await client.db("admin").command({ ping: 1 });
  } catch (e) {
    await client.close().catch(() => undefined);
    throw new Error(`Cannot reach MongoDB at ${redact(uri())}: ${redact((e as Error).message).split(uri()).join("<uri>")}`);
  }
  return client;
}

const stamp = (d: Date) => d.toISOString().replace(/[-:]/g, "").replace(/\.\d+Z$/, "Z");

async function collectionNames(db: Db): Promise<string[]> {
  const infos = await db.listCollections({ type: "collection" }, { nameOnly: true }).toArray();
  return infos.map((i) => i.name).filter((n) => !n.startsWith("system.")).sort();
}

export async function dumpDb(db: Db, name: string): Promise<{ dir: string; manifest: Manifest }> {
  const root = backupRoot();
  await mkdir(root, { recursive: true, mode: 0o700 });
  const now = new Date();
  const dir = path.join(root, `${name}-${stamp(now)}`);
  const tmp = `${dir}.partial`;
  await mkdir(tmp, { mode: 0o700 });
  const manifest: Manifest = { format: FORMAT, db: name, createdAt: now.toISOString(), collections: {} };
  try {
    for (const coll of await collectionNames(db)) {
      const file = `${encodeURIComponent(coll)}.jsonl.gz`;
      const cursor = db.collection(coll).find({});
      let count = 0;
      async function* lines() {
        for await (const doc of cursor) {
          count++;
          yield EJSON.stringify(doc, { relaxed: false }) + "\n";
        }
      }
      const out = createWriteStream(path.join(tmp, file), { mode: 0o600 });
      await pipeline(Readable.from(lines()), createGzip(), out);
      const indexes = (await db.collection(coll).indexes()).filter((i) => i.name !== "_id_");
      manifest.collections[coll] = { count, file, indexes: indexes as Document[] };
    }
    await writeFile(path.join(tmp, "manifest.json"), JSON.stringify(manifest, null, 2), { mode: 0o600 });
    await rename(tmp, dir); // a backup directory only exists once it is complete
  } catch (e) {
    await rm(tmp, { recursive: true, force: true });
    throw e;
  }
  await chmod(dir, 0o700);
  return { dir, manifest };
}

export async function readManifest(dir: string): Promise<Manifest> {
  const m = JSON.parse(await readFile(path.join(dir, "manifest.json"), "utf8")) as Manifest;
  if (m.format !== FORMAT) throw new Error(`Unsupported backup format ${m.format}`);
  return m;
}

export async function loadInto(db: Db, dir: string, manifest: Manifest): Promise<Record<string, number>> {
  const loaded: Record<string, number> = {};
  for (const [coll, info] of Object.entries(manifest.collections)) {
    const rl = createInterface({ input: createReadStream(path.join(dir, info.file)).pipe(createGunzip()), crlfDelay: Infinity });
    let batch: Document[] = [];
    let n = 0;
    const flush = async () => {
      if (batch.length) await db.collection(coll).insertMany(batch, { ordered: true });
      n += batch.length;
      batch = [];
    };
    for await (const line of rl) {
      if (!line) continue;
      batch.push(EJSON.parse(line, { relaxed: false }) as Document);
      if (batch.length >= 1000) await flush();
    }
    await flush();
    if (!n) await db.createCollection(coll).catch(() => undefined);
    for (const ix of info.indexes) {
      const { key, ...opts } = ix as Document;
      delete opts.v; delete opts.ns;
      await db.collection(coll).createIndex(key, opts);
    }
    loaded[coll] = n;
  }
  return loaded;
}

export async function countAll(db: Db): Promise<Record<string, number>> {
  const out: Record<string, number> = {};
  for (const c of await collectionNames(db)) out[c] = await db.collection(c).countDocuments();
  return out;
}

export function compareCounts(expected: Record<string, number>, actual: Record<string, number>): string[] {
  const problems: string[] = [];
  for (const [c, n] of Object.entries(expected)) {
    if (actual[c] !== n) problems.push(`${c}: expected ${n}, found ${actual[c] ?? "missing"}`);
  }
  return problems;
}

/** Restore into a throwaway database, compare counts, always drop it. */
export async function verifyBackup(client: MongoClient, dir: string): Promise<{ problems: string[]; table: string[] }> {
  const manifest = await readManifest(dir);
  const scratch = client.db(`${manifest.db}_verify_${Date.now().toString(36)}`);
  try {
    await loadInto(scratch, dir, manifest);
    const actual = await countAll(scratch);
    const expected = Object.fromEntries(Object.entries(manifest.collections).map(([c, i]) => [c, i.count]));
    const table = Object.keys(expected).map((c) => `  ${c.padEnd(24)} backup=${String(expected[c]).padStart(7)} restored=${String(actual[c] ?? 0).padStart(7)}`);
    return { problems: compareCounts(expected, actual), table };
  } finally {
    await scratch.dropDatabase();
  }
}

/** Delete all but the newest `keep` complete backups of this db in BACKUP_DIR. */
export async function prune(name: string, keep: number): Promise<string[]> {
  const root = backupRoot();
  const re = new RegExp(`^${name.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}-\\d{8}T\\d{6}Z$`);
  const dirs = (await readdir(root)).filter((d) => re.test(d)).sort();
  const old = dirs.slice(0, Math.max(0, dirs.length - keep));
  for (const d of old) await rm(path.join(root, d), { recursive: true, force: true });
  return old;
}
