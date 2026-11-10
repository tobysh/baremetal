// Shared helpers for backup-db.mts and restore-db.mts.
// Format: <BACKUP_DIR>/<db>-<UTC timestamp>/ containing manifest.json plus one
// <collection>.jsonl.gz per collection (one canonical Extended JSON document per
// line, so ObjectId, Date and numeric types survive). Indexes are in the manifest.
import { createReadStream, createWriteStream } from "node:fs";
import { chmod, lstat, mkdir, readdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import { createHash, randomBytes } from "node:crypto";
import { homedir } from "node:os";
import path from "node:path";
import { createGunzip, createGzip } from "node:zlib";
import { pipeline } from "node:stream/promises";
import { Readable, Transform } from "node:stream";
import { StringDecoder } from "node:string_decoder";
import { BSON, MongoClient, type Db, type Document } from "mongodb";

const { EJSON } = BSON;

// Format 2 introduces digests and canonical-EJSON index metadata. Old format-1
// restorers must reject it before touching a destination; new readers retain format 1.
export const FORMAT = 2;
export const uri = () => process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017";
export const dbName = () => process.env.MONGODB_DB ?? "baremetal";
export const backupRoot = () => path.resolve(process.env.BACKUP_DIR ?? path.join(homedir(), "baremetal-backups"));

export type Manifest = {
  format: number;
  db: string;
  createdAt: string;
  collections: Record<string, { count: number; file: string; indexes: Document[]; sha256?: string; documentsSha256?: string; indexEncoding?: "canonical-ejson" }>;
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
  const manifest: Manifest = { format: FORMAT, db: name, createdAt: now.toISOString(), collections: Object.create(null) };
  try {
    for (const coll of await collectionNames(db)) {
      const file = `${encodeURIComponent(coll)}.jsonl.gz`;
      // Default driver promotion can turn a BSON Double or small Long into a JS
      // number and subsequently restore it as Int32. Preserve the original types.
      const cursor = db.collection(coll).find({}, { promoteValues: false }).collation({ locale: "simple" }).sort({ _id: 1 });
      const documentsHash = createHash("sha256");
      const fileHash = createHash("sha256");
      let count = 0;
      async function* lines() {
        for await (const doc of cursor) {
          count++;
          const line = EJSON.stringify(doc, { relaxed: false }) + "\n";
          documentsHash.update(line);
          yield line;
        }
      }
      const out = createWriteStream(path.join(tmp, file), { mode: 0o600 });
      await pipeline(Readable.from(lines()), createGzip(), new Transform({ transform(chunk, _encoding, done) { fileHash.update(chunk); done(null, chunk); } }), out);
      const indexes = (await db.collection(coll).indexes()).filter((i) => i.name !== "_id_");
      manifest.collections[coll] = { count, file, indexes: EJSON.serialize(indexes, { relaxed: false }) as Document[], indexEncoding: "canonical-ejson", sha256: fileHash.digest("hex"), documentsSha256: documentsHash.digest("hex") };
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
  if (!(await lstat(path.join(dir, "manifest.json"))).isFile()) throw new Error("Backup manifest must be a regular file, not a symbolic link");
  const m = JSON.parse(await readFile(path.join(dir, "manifest.json"), "utf8")) as Manifest;
  validateManifest(m);
  return m;
}

function validateManifest(m: Manifest): void {
  if (!m || typeof m !== "object") throw new Error("Invalid backup manifest");
  if (m.format !== 1 && m.format !== FORMAT) throw new Error(`Unsupported backup format ${m.format}`);
  if (typeof m.db !== "string" || !m.db || typeof m.createdAt !== "string" || !Number.isFinite(Date.parse(m.createdAt)) || !m.collections || typeof m.collections !== "object" || Array.isArray(m.collections)) throw new Error("Invalid backup manifest");
  for (const [coll, info] of Object.entries(m.collections)) {
    if (!coll || coll.includes("\0") || coll.startsWith("system.") || !info || typeof info !== "object" || !Number.isSafeInteger(info.count) || info.count < 0 || typeof info.file !== "string" || /[\\/]/.test(info.file) || !info.file.endsWith(".jsonl.gz") || !Array.isArray(info.indexes)) throw new Error(`Invalid backup collection entry: ${coll}`);
    for (const digest of [info.sha256, info.documentsSha256]) if (digest !== undefined && (typeof digest !== "string" || !/^[a-f0-9]{64}$/.test(digest))) throw new Error(`Invalid backup digest: ${coll}`);
    if (info.indexEncoding !== undefined && info.indexEncoding !== "canonical-ejson") throw new Error(`Unsupported index encoding: ${coll}`);
    for (const index of info.indexes) if (!index || typeof index !== "object" || !index.key || typeof index.key !== "object" || Array.isArray(index.key)) throw new Error(`Invalid backup index: ${coll}`);
  }
}

async function* documents(file: string): AsyncGenerator<Document> {
  const input = createReadStream(file).compose(createGunzip());
  const decoder = new StringDecoder("utf8");
  let pending = "";
  const parse = (line: string) => {
    const doc = EJSON.parse(line, { relaxed: false }) as unknown;
    if (!doc || typeof doc !== "object" || Array.isArray(doc)) throw new Error("Backup line must be a document");
    return doc as Document;
  };
  try {
    for await (const chunk of input) {
      pending += decoder.write(chunk as Buffer);
      let end: number;
      while ((end = pending.indexOf("\n")) !== -1) {
        const line = pending.slice(0, end);
        pending = pending.slice(end + 1);
        if (line.trim()) yield parse(line);
      }
    }
    pending += decoder.end();
    if (pending.trim()) yield parse(pending);
  } finally { input.destroy(); }
}

/** Check every artifact before any destination collection can be dropped or written.
 * Digests detect corruption, not an attacker who can also rewrite the manifest. */
export async function validateBackup(dir: string, manifest: Manifest): Promise<void> {
  validateManifest(manifest);
  for (const [coll, info] of Object.entries(manifest.collections)) {
    const file = path.join(dir, info.file);
    if (!(await lstat(file)).isFile()) throw new Error(`Backup artifact must be a regular file: ${coll}`);
    if (info.sha256) {
      const hash = createHash("sha256");
      for await (const chunk of createReadStream(file)) hash.update(chunk);
      if (hash.digest("hex") !== info.sha256) throw new Error(`Backup SHA-256 mismatch: ${coll}`);
    }
    const hash = createHash("sha256");
    let count = 0;
    for await (const doc of documents(file)) { count++; hash.update(EJSON.stringify(doc, { relaxed: false }) + "\n"); }
    if (count !== info.count) throw new Error(`Backup document count mismatch: ${coll}`);
    if (info.documentsSha256 && hash.digest("hex") !== info.documentsSha256) throw new Error(`Backup document digest mismatch: ${coll}`);
  }
}

export async function loadInto(db: Db, dir: string, manifest: Manifest): Promise<Record<string, number>> {
  await validateBackup(dir, manifest);
  const loaded: Record<string, number> = Object.create(null);
  for (const [coll, info] of Object.entries(manifest.collections)) {
    let batch: Document[] = [];
    let n = 0;
    const flush = async () => {
      if (batch.length) await db.collection(coll).insertMany(batch, { ordered: true });
      n += batch.length;
      batch = [];
    };
    for await (const doc of documents(path.join(dir, info.file))) {
      batch.push(doc);
      if (batch.length >= 1000) await flush();
    }
    await flush();
    if (!n) await db.createCollection(coll).catch(() => undefined);
    const indexes = info.indexEncoding === "canonical-ejson" ? EJSON.deserialize(info.indexes, { relaxed: false }) as Document[] : info.indexes;
    for (const ix of indexes) {
      const { key, ...opts } = ix as Document;
      delete opts.v; delete opts.ns;
      await db.collection(coll).createIndex(key, opts);
    }
    loaded[coll] = n;
  }
  return loaded;
}

export async function countAll(db: Db): Promise<Record<string, number>> {
  const out: Record<string, number> = Object.create(null);
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

/** Restore into a throwaway database, compare counts and available BSON digests, always drop it. */
export async function verifyBackup(client: MongoClient, dir: string): Promise<{ problems: string[]; table: string[] }> {
  const manifest = await readManifest(dir);
  await validateBackup(dir, manifest);
  const scratch = client.db(`${manifest.db.slice(0, 24)}_verify_${randomBytes(12).toString("hex")}`);
  if (await scratch.listCollections({}, { nameOnly: true }).hasNext()) throw new Error("Verification scratch database already exists; refusing to replace it");
  try {
    await loadInto(scratch, dir, manifest);
    const actual = await countAll(scratch);
    const expected = Object.fromEntries(Object.entries(manifest.collections).map(([c, i]) => [c, i.count]));
    const table = Object.keys(expected).map((c) => `  ${c.padEnd(24)} backup=${String(expected[c]).padStart(7)} restored=${String(actual[c] ?? 0).padStart(7)}`);
    const problems = compareCounts(expected, actual);
    for (const [coll, info] of Object.entries(manifest.collections)) {
      if (!info.documentsSha256) continue;
      const hash = createHash("sha256");
      for await (const doc of scratch.collection(coll).find({}, { promoteValues: false }).collation({ locale: "simple" }).sort({ _id: 1 })) hash.update(EJSON.stringify(doc, { relaxed: false }) + "\n");
      if (hash.digest("hex") !== info.documentsSha256) problems.push(`${coll}: restored BSON content differs from the backup`);
    }
    return { problems, table };
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
