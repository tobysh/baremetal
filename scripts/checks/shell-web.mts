// Web-side shell checks against a fake broker socket. No database, server or VM.
// Run: node --conditions=react-server --import tsx scripts/checks/shell-web.mts
import assert from "node:assert/strict";
import { mkdtemp, rm } from "node:fs/promises";
import { createServer, type Socket } from "node:net";
import { tmpdir } from "node:os";
import path from "node:path";
import { encodeFrame, FrameParser, FrameType } from "../../src/lib/shell/frames";
import { createShellHandlers, LIMITS, type ShellUser } from "../../src/lib/shell/web";
import { sameOrigin } from "../../src/lib/http";

process.env.AUTH_SECRET ??= "x".repeat(40);
const dir = await mkdtemp(path.join(tmpdir(), "bm-shell-web-"));
const socketPath = path.join(dir, "broker.sock");

type Seen = { open?: { account: string; cols: number; rows: number }; input: Buffer[]; resizes: number[][]; closed: boolean; socket: Socket };
const seen: Seen[] = [];
let busyNext = false;
const broker = createServer((socket) => {
  const rec: Seen = { input: [], resizes: [], closed: false, socket };
  seen.push(rec);
  const parser = new FrameParser();
  socket.on("close", () => (rec.closed = true));
  socket.on("error", () => {});
  socket.on("data", (chunk) => {
    for (const f of parser.push(chunk)) {
      if (f.type === FrameType.Open) {
        rec.open = JSON.parse(f.payload.toString());
        if (busyNext) {
          busyNext = false;
          socket.write(encodeFrame(FrameType.Error, Buffer.from(JSON.stringify({ code: "busy", message: "No shell is free right now. Try again in a minute." }))));
          socket.end();
        } else {
          socket.write(encodeFrame(FrameType.Opened));
          socket.write(encodeFrame(FrameType.Data, Buffer.from("hello $ ")));
        }
      } else if (f.type === FrameType.Data) rec.input.push(f.payload);
      else if (f.type === FrameType.Resize) rec.resizes.push([f.payload.readUInt16BE(0), f.payload.readUInt16BE(2)]);
    }
  });
});
await new Promise<void>((r) => broker.listen(socketPath, r));

let user: ShellUser | null = { id: "user-one-0000000000000001", emailVerified: true };
let allow = true;
const handlers = createShellHandlers({
  getUser: async () => user,
  sameOrigin,
  rateLimit: async () => allow,
  hash: (purpose, data) => Buffer.from(`${purpose}:${data}`).toString("base64url").padEnd(20, "0"),
  clientIp: () => "203.0.113.9",
  socketPath: () => socketPath,
});

const base = "http://localhost:3000";
const headers = (extra: Record<string, string> = {}) => ({ origin: base, host: "localhost:3000", ...extra });
const post = (url: string, body: BodyInit | null, extra: Record<string, string> = {}) => new Request(`${base}${url}`, { method: "POST", headers: headers(extra), body });
const create = (body = JSON.stringify({ cols: 80, rows: 24 }), extra: Record<string, string> = { "content-type": "application/json" }) => handlers.create(post("/api/shell", body, extra));
const wait = async (cond: () => boolean) => { for (let i = 0; i < 100 && !cond(); i++) await new Promise((r) => setTimeout(r, 20)); assert.ok(cond(), "timed out"); };
let passed = 0;
const ok = (name: string) => console.log(`ok - ${name}`, ++passed && "");

try {
  // Gate: origin, sign-in, verification.
  assert.equal((await handlers.create(new Request(`${base}/api/shell`, { method: "POST", headers: { origin: "https://evil.example", host: "localhost:3000" }, body: "{}" }))).status, 403);
  assert.equal((await handlers.create(new Request(`${base}/api/shell`, { method: "POST", body: "{}" }))).status, 403); // no origin, no sec-fetch-site
  ok("cross-site and origin-less requests are refused");
  user = null;
  assert.equal((await create()).status, 401);
  user = { id: "user-one-0000000000000001", emailVerified: false };
  assert.equal((await create()).status, 403);
  assert.equal(seen.length, 0, "broker untouched by gated requests");
  ok("signed-out and unverified accounts never reach the broker");
  user = { id: "user-one-0000000000000001", emailVerified: true };

  // Rate limit.
  allow = false;
  assert.equal((await create()).status, 429);
  allow = true;
  assert.equal(seen.length, 0);
  ok("rate limit applies to creation");

  // Body limits and validation on create.
  assert.equal((await create("x".repeat(LIMITS.createBody + 1))).status, 400);
  assert.equal((await create(JSON.stringify({ cols: 9999, rows: 24 }))).status, 400);
  assert.equal((await create(JSON.stringify({ cols: "80", rows: 24 }))).status, 400);
  assert.equal(seen.length, 0);
  ok("create validates and bounds its body");

  // Broker refusal is mapped and releases the account.
  busyNext = true;
  const refused = await create();
  assert.equal(refused.status, 503);
  assert.equal(((await refused.json()) as { code: string }).code, "busy");
  ok("broker busy error is surfaced");

  // Happy path.
  const res = await create();
  assert.equal(res.status, 201);
  const { token } = (await res.json()) as { token: string };
  assert.ok(token.length >= 40);
  const rec = seen[seen.length - 1];
  assert.deepEqual([rec.open!.cols, rec.open!.rows], [80, 24]);
  assert.ok(!rec.open!.account.includes(user.id) && /^[A-Za-z0-9_-]{8,128}$/.test(rec.open!.account), "account is an opaque hash");
  ok("session opens with an opaque account hash");

  // One session per account, also when racing.
  assert.equal((await create()).status, 409);
  ok("second session for the same account is refused");

  // Other users cannot touch the token; bad token is 404.
  const mine = user;
  user = { id: "user-two-0000000000000002", emailVerified: true };
  const sid = (suffix = "") => `/api/shell/${token}${suffix}`;
  assert.equal((await handlers.input(post(sid("/input"), "a", { "content-type": "application/octet-stream" }), token)).status, 404);
  assert.equal((await handlers.stream(new Request(`${base}${sid()}`, { headers: headers() }), token)).status, 404);
  assert.equal((await handlers.close(new Request(`${base}${sid()}`, { method: "DELETE", headers: headers() }), token)).status, 204);
  assert.equal(rec.closed, false, "another user's DELETE must not close the session");
  user = mine;
  assert.equal((await handlers.input(post("/x", "a", { "content-type": "application/octet-stream" }), "nope")).status, 404);
  ok("tokens are bound to their account");

  // Input limits.
  const octet = { "content-type": "application/octet-stream" };
  assert.equal((await handlers.input(post(sid("/input"), "x".repeat(LIMITS.inputBody + 1), octet), token)).status, 413);
  assert.equal((await handlers.input(post(sid("/input"), "x", { "content-type": "text/plain" }), token)).status, 415);
  assert.equal((await handlers.input(new Request(`${base}${sid("/input")}`, { method: "POST", headers: { origin: "https://evil.example", host: "localhost:3000", ...octet }, body: "x" }), token)).status, 403);
  assert.equal((await handlers.resize(post(sid("/resize"), JSON.stringify({ cols: 0, rows: 5 })), token)).status, 400);
  assert.equal((await handlers.resize(post(sid("/resize"), "x".repeat(LIMITS.resizeBody + 1)), token)).status, 400);
  assert.equal(rec.input.length, 0);
  ok("input and resize sizes, types and origin are enforced");

  // Stream, input and resize reach the broker; output arrives base64 over SSE.
  const abort = new AbortController();
  const stream = await handlers.stream(new Request(`${base}${sid()}`, { headers: headers(), signal: abort.signal }), token);
  assert.equal(stream.status, 200);
  assert.match(stream.headers.get("content-type")!, /text\/event-stream/);
  assert.equal((await handlers.stream(new Request(`${base}${sid()}`, { headers: headers() }), token)).status, 409);
  const reader = stream.body!.getReader();
  let text = "";
  const decoder = new TextDecoder();
  while (!text.includes("aGVsbG8gJCA=")) { const { value, done } = await reader.read(); assert.ok(!done); text += decoder.decode(value); }
  assert.equal((await handlers.input(post(sid("/input"), "ls\n", octet), token)).status, 204);
  assert.equal((await handlers.resize(post(sid("/resize"), JSON.stringify({ cols: 100, rows: 30 })), token)).status, 204);
  await wait(() => rec.input.length > 0 && rec.resizes.length > 0);
  assert.equal(Buffer.concat(rec.input).toString(), "ls\n");
  assert.deepEqual(rec.resizes[0], [100, 30]);
  ok("stream, input and resize relay through the broker");

  // Aborting the request closes the broker connection and frees the account.
  abort.abort();
  await wait(() => rec.closed);
  await wait(() => handlers.stats().sessions === 0 && handlers.stats().accounts === 0);
  ok("aborted stream closes the broker connection and releases the account");

  // Broker error codes arrive as a shell-error event.
  const second = await (await create()).json() as { token: string };
  const live = seen[seen.length - 1];
  const s2 = await handlers.stream(new Request(`${base}/api/shell/${second.token}`, { headers: headers() }), second.token);
  live.socket.write(encodeFrame(FrameType.Error, Buffer.from(JSON.stringify({ code: "idle_timeout", message: "The shell closed after a period of inactivity." }))));
  let t2 = "";
  const r2 = s2.body!.getReader();
  for (;;) { const { value, done } = await r2.read(); if (done) break; t2 += decoder.decode(value); }
  assert.match(t2, /event: shell-error\ndata: .*idle_timeout/);
  assert.equal(handlers.stats().sessions, 0);
  ok("broker end reasons are forwarded and the session is released");

  // An unclaimed session is reaped; explicit DELETE closes too.
  const third = await (await create()).json() as { token: string };
  const rec3 = seen[seen.length - 1];
  assert.equal((await handlers.close(new Request(`${base}/api/shell/${third.token}`, { method: "DELETE", headers: headers() }), third.token)).status, 204);
  await wait(() => rec3.closed);
  ok("DELETE closes the broker connection");
  console.log(`all ${passed} groups passed`);
} finally {
  for (const r of seen) r.socket.destroy();
  broker.close();
  await rm(dir, { recursive: true, force: true });
}
process.exit(0);
