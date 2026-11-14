// Run with: node --conditions=react-server --import tsx scripts/tutor/abort-checks.mts   (needs MongoDB; no API key)
// A fake Anthropic server sends message_start (input_tokens=1000) and then stalls; the client aborts mid-stream.
// The aborted call must still be charged for its input tokens, and must report that the upstream call was made.
import assert from "node:assert/strict";
import http from "node:http";
import type { AddressInfo } from "node:net";
import { MongoClient, ObjectId } from "mongodb";

const server = http.createServer((req, res) => {
  req.resume();
  res.writeHead(200, { "content-type": "text/event-stream" });
  const start = { type: "message_start", message: { id: "msg_1", type: "message", role: "assistant", model: "claude-haiku-5-5", content: [], stop_reason: null, stop_sequence: null, usage: { input_tokens: 1000, output_tokens: 1, cache_read_input_tokens: 0, cache_creation_input_tokens: 0 } } };
  res.write(`event: message_start\ndata: ${JSON.stringify(start)}\n\n`);
  const block = { type: "content_block_start", index: 0, content_block: { type: "text", text: "" } };
  res.write(`event: content_block_start\ndata: ${JSON.stringify(block)}\n\n`);
  const delta = { type: "content_block_delta", index: 0, delta: { type: "text_delta", text: "Let's think" } };
  res.write(`event: content_block_delta\ndata: ${JSON.stringify(delta)}\n\n`);
  // ...and never finishes.
});
await new Promise<void>((r) => server.listen(0, "127.0.0.1", r));
process.env.ANTHROPIC_BASE_URL = `http://127.0.0.1:${(server.address() as AddressInfo).port}`;
process.env.ANTHROPIC_API_KEY = "test";
// This fixture always exercises its local fake Anthropic endpoint, regardless of the app provider.
process.env.TUTOR_PROVIDER = "anthropic";
process.env.TUTOR_BUDGET_USER_DAY_USD = "1";
process.env.TUTOR_BUDGET_GLOBAL_DAY_USD = "10";

import type { TutorUsage } from "../../src/lib/tutor";
const { streamTutor } = await import("../../src/lib/tutor");
const { markTutorUsed } = await import("../../src/lib/tutor-used");
const { reserveBudget } = await import("../../src/lib/tutor-budget");
const client = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", { serverSelectionTimeoutMS: 5000 });
const db = client.db(`bm_abort_check_${process.pid}_${Date.now()}`);
try {
  await client.connect();
  await db.collection("tutor_spend").createIndex({ key: 1 }, { unique: true });
  const userId = new ObjectId();
  await db.collection("users").insertOne({ _id: userId });
  const settle = await reserveBudget(db, userId.toHexString(), 500_000);
  assert(settle, "reservation granted");

  const abort = new AbortController();
  let upstream = false;
  let pending: Promise<void> | undefined; // the route's onUpstream hook records tutorUsed
  const seen: { started: TutorUsage | null } = { started: null };
  const chunks = streamTutor("ctx", [{ role: "user", content: "help" }], abort.signal, () => {}, { onUpstream: () => { upstream = true; pending = markTutorUsed(db, "0123456789abcdef01234567", { id: "c/x", trackSlug: "c" }, "int main(){}"); }, onStart: (u) => { seen.started = u; } });
  let got = "";
  try {
    for await (const t of chunks) { got += t; abort.abort(); }
  } catch { /* aborted, as the route sees it */ }
  assert.equal(got, "Let's think", "partial text streamed before the abort");
  assert(upstream, "upstream call is flagged, so the route must not refund the quota");
  assert.equal(seen.started?.inputTokens, 1000, "input_tokens read from message_start");

  await settle(seen.started); // what the route's finally block does
  const row = await db.collection("tutor_spend").findOne({ key: new RegExp(`^u:${userId.toHexString()}:`) });
  assert(row && row.micro > 0, `aborted call still costs its input tokens (held ${row?.micro})`);
  await pending;
  const prog = await db.collection("progress").findOne({ exerciseId: "c/x" });
  assert.equal(prog?.tutorUsed, true, "tutorUsed is set after an aborted request");
  console.log("Tutor abort checks passed: message_start input charged, upstream flagged, tutorUsed set.");
} finally {
  await db.dropDatabase();
  await client.close();
  server.closeAllConnections();
  server.close();
}
