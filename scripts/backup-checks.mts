import assert from "node:assert/strict";
import { mkdtemp, rm } from "node:fs/promises";
import { MongoClient, ObjectId, Long } from "mongodb";
import { dumpDb, loadInto, verifyBackup } from "./lib/backup.mjs";
const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", {serverSelectionTimeoutMS: 5000});
const name = `bm_backup_check_${process.pid}_${Date.now()}`;
const source = client.db(name), restored = client.db(`${name}_restored`);
const root = await mkdtemp('/tmp/baremetal-backup-check-'); process.env.BACKUP_DIR = root;
try {
  await client.connect();
  const original = {_id: new ObjectId(), at: new Date("2026-01-02T00:00:00Z"), amount: Long.fromString("9007199254740993"), text: "hello"};
  await source.collection("docs").insertOne(original);
  await source.collection("docs").createIndex({text: 1}, {unique: true});
  await source.collection("docs").createIndex({at: 1}, {expireAfterSeconds: 315360000});
  await source.createCollection("empty");
  const {dir, manifest} = await dumpDb(source, name);
  assert.deepEqual((await verifyBackup(client,dir)).problems, []);
  await loadInto(restored,dir,manifest);
  const doc = await restored.collection("docs").findOne({});
  assert(doc?._id.equals(original._id));
  assert.equal(doc?.at.getTime(),original.at.getTime());
  assert.equal(doc?.amount.toString(),original.amount.toString());
  const indexes = await restored.collection("docs").indexes();
  assert(indexes.some((i) => i.unique && i.key.text === 1));
  assert(indexes.some((i) => i.expireAfterSeconds === 315360000));
  assert.equal(await restored.collection("empty").countDocuments(),0);
  console.log("Backup/restore counts, BSON types, empty collection, unique and TTL indexes passed.");
} finally {
  await source.dropDatabase(); await restored.dropDatabase(); await client.close();
  await rm(root,{recursive:true,force:true});
}
