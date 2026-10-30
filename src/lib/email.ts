import "server-only";
import nodemailer from "nodemailer";

export async function sendEmail(to: string, subject: string, text: string) {
  const smtp = process.env.SMTP_URL;
  if (!smtp) {
    if (process.env.NODE_ENV === "production") throw new Error("SMTP_URL is not configured");
    // Development fallback: print the email so the flow can be tested locally.
    console.info(`\n[email:dev] To: ${to}\nSubject: ${subject}\n\n${text}\n`);
    return;
  }
  const transport = nodemailer.createTransport(smtp);
  await transport.sendMail({ from: process.env.EMAIL_FROM ?? "no-reply@example.com", to, subject, text });
}

// The payload contains reset/verification links, so keep it encrypted at rest.
// A lease allows a later worker to retry after a crash; delivery is at least once.
import { after } from "next/server";
import { getDb } from "./db";
import { decrypt, encrypt, hmac } from "./crypto";

export async function queueEmail(to: string, subject: string, text: string) {
  const db = await getDb();
  await db.collection("email_queue").insertOne({ recipientHash: hmac("email-queue-recipient", to.toLowerCase()), payload: encrypt(JSON.stringify({ to, subject, text })), createdAt: new Date(), nextAttempt: new Date(), expiresAt: new Date(Date.now() + 7 * 86400_000), attempts: 0 });
}

/** Schedule delivery after the response. Pending messages are retried by future runs. */
export function scheduleEmailDelivery() {
  after(async () => { await drainEmailQueue().catch((e) => console.error("[email] queue worker failed", e)); });
}
const MAX_EMAIL_ATTEMPTS = 10;
export async function drainEmailQueue() {
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
