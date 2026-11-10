import "server-only";
import nodemailer, { type Transporter } from "nodemailer";

// One pooled transport per process, with timeouts so a stalled SMTP server cannot hold a worker open.
let pooled: { url: string; transport: Transporter } | undefined;
function transportFor(url: string) {
  if (pooled?.url !== url) {
    pooled?.transport.close();
    pooled = { url, transport: nodemailer.createTransport({ url, pool: true, maxConnections: 2, maxMessages: 100, connectionTimeout: 10_000, greetingTimeout: 10_000, socketTimeout: 20_000, dnsTimeout: 10_000 }) };
  }
  return pooled.transport;
}

export async function sendEmail(to: string, subject: string, text: string) {
  const smtp = process.env.SMTP_URL;
  if (!smtp) {
    if (process.env.NODE_ENV === "production") throw new Error("SMTP_URL is not configured");
    // Development fallback: print the email so the flow can be tested locally.
    console.info(`\n[email:dev] To: ${to}\nSubject: ${subject}\n\n${text}\n`);
    return;
  }
  await transportFor(smtp).sendMail({ from: process.env.EMAIL_FROM ?? "no-reply@example.com", to, subject, text });
}

// The payload contains reset/verification links, so keep it encrypted at rest.
// A lease allows a later worker to retry after a crash; delivery is at least once.
import { after } from "next/server";
import { getDb } from "./db";
import { decrypt, encrypt, hmac } from "./crypto";

const RECIPIENT_QUEUE_LIMIT = 10; // queued messages per recipient per hour, whatever the caller
export async function queueEmail(to: string, subject: string, text: string) {
  const recipientHash = hmac("email-queue-recipient", to.toLowerCase());
  // Backstop for every caller: one address cannot be flooded, and the limiter must not reveal anything to the requester.
  const { rateLimit } = await import("./rate-limit");
  if (!(await rateLimit(`email-queue:${recipientHash}`, RECIPIENT_QUEUE_LIMIT, 3600))) { console.warn("[email] per-recipient queue limit reached; message not queued"); return; }
  const db = await getDb();
  await db.collection("email_queue").insertOne({ recipientHash, payload: encrypt(JSON.stringify({ to, subject, text })), createdAt: new Date(), nextAttempt: new Date(), expiresAt: new Date(Date.now() + 7 * 86400_000), attempts: 0 });
}

/** Schedule delivery after the response. Pending messages are retried by future runs. */
export function scheduleEmailDelivery() {
  after(async () => { await drainEmailQueue().catch((e) => console.error("[email] queue worker failed", e)); });
}
const MAX_EMAIL_ATTEMPTS = 10;
// Only one drainer per process (and, via a database lease, per deployment) at a time, so overlapping
// after() callbacks and the timer cannot hammer SMTP or double-send.
let draining = false;
const DRAIN_LEASE_MS = 15 * 60_000;
export async function drainEmailQueue() {
  if (draining) return;
  draining = true;
  const db = await getDb();
  const lock = db.collection<{ _id: string; until: Date }>("locks");
  const now0 = new Date();
  const got = await lock.findOneAndUpdate(
    { _id: "email-drain", until: { $lte: now0 } },
    { $set: { until: new Date(now0.getTime() + DRAIN_LEASE_MS) } },
    { returnDocument: "after" },
  ).catch(() => null) ?? await lock.insertOne({ _id: "email-drain", until: new Date(now0.getTime() + DRAIN_LEASE_MS) }).then(() => true, () => null);
  if (!got) { draining = false; return; }
  try {
    await drainLocked();
  } finally {
    await lock.updateOne({ _id: "email-drain" }, { $set: { until: new Date(0) } }).catch(() => {});
    draining = false;
  }
}
async function drainLocked() {
  const col = (await getDb()).collection("email_queue");
  for (let i = 0; i < 25; i++) {
    const now = new Date();
    const job = await col.findOneAndUpdate(
      { nextAttempt: { $lte: now }, expiresAt: { $gt: now } },
      { $set: { nextAttempt: new Date(now.getTime() + 5 * 60_000) }, $inc: { attempts: 1 } },
      { sort: { createdAt: 1 }, returnDocument: "after" },
    );
    if (!job) break;
    try {
      const data = JSON.parse(decrypt(job.payload)) as { to: string; subject: string; text: string };
      await sendEmail(data.to, data.subject, data.text);
      await col.deleteOne({ _id: job._id });
    } catch (e) {
      // Exponential backoff (1, 2, 4 ... up to 60 min), then give up so a dead address or provider can't be retried for days.
      const attempts = job.attempts ?? 1;
      if (attempts >= MAX_EMAIL_ATTEMPTS) {
        await col.deleteOne({ _id: job._id });
        console.error(`[email] dropped queued email ${job._id} after ${attempts} failed attempts`, e);
      } else {
        const minutes = Math.min(60, 2 ** (attempts - 1));
        await col.updateOne({ _id: job._id }, { $set: { nextAttempt: new Date(Date.now() + minutes * 60_000) } });
        console.error(`[email] queued delivery failed (attempt ${attempts}), retrying in ${minutes} min`, e);
      }
    }
  }
}
