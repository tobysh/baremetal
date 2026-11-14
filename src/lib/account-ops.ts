import "server-only";
import type { ObjectId } from "mongodb";
import { getDb } from "./db";
import { hmac, randomToken, sha256 } from "./crypto";
import { queueEmail } from "./email";
import { deleteTutorUsage } from "./tutor-budget";
import { killUser as killShells } from "./shell/kill";
import type { UserDoc } from "./models";
import { absoluteUrl } from "./site";

export const RESET_MINUTES = 30;

/** Replaces any pending reset link with a new one and queues the email. The caller schedules delivery. */
export async function issuePasswordReset(user: Pick<UserDoc, "_id" | "email" | "name">, requestedIp?: string) {
  const db = await getDb();
  const token = randomToken(32);
  await db.collection("password_resets").deleteMany({ userId: user._id });
  await db.collection("password_resets").insertOne({
    tokenHash: sha256(token),
    userId: user._id,
    ...(requestedIp ? { requestedIp: hmac("rate-ip", requestedIp) } : {}),
    expiresAt: new Date(Date.now() + RESET_MINUTES * 60_000),
  });
  // Delivery failures are logged, never surfaced to the requester: a different response for
  // registered emails would reveal which accounts exist.
  await queueEmail(user.email, "Reset your Baremetal password", `Hi ${user.name},\n\nA link to reset your password was requested. Use it within ${RESET_MINUTES} minutes:\n\n${absoluteUrl("/reset-password")}#${token}\n\nIf you didn't ask for this, ignore this email.`);
}

/** Replaces any pending verification link with a new one (valid 24 hours) and queues the email. */
export async function issueEmailVerification(user: Pick<UserDoc, "_id" | "email">, intro = "Use this link within 24 hours to verify your email and choose your password:") {
  const db = await getDb();
  const token = randomToken(32);
  await db.collection("email_verifications").deleteMany({ userId: user._id });
  await db.collection("email_verifications").insertOne({ tokenHash: sha256(token), userId: user._id, expiresAt: new Date(Date.now() + 24 * 3600_000) });
  await queueEmail(user.email, "Verify your Baremetal email", `${intro}\n\n${absoluteUrl("/verify-email")}#${token}\n\nIf you did not expect this, ignore this email.`);
}

/** Ends every session for the user, then removes the account and everything tied to it. */
export async function deleteUserData(user: Pick<UserDoc, "_id" | "email">) {
  const db = await getDb();
  const id: ObjectId = user._id;
  // The account and its sessions go first, so getCurrentUser() returns null from here on and an
  // in-flight autosave or tutor turn can't recreate rows behind the cleanup below.
  await db.collection("sessions").deleteMany({ userId: id });
  await db.collection("users").deleteOne({ _id: id });
  killShells(id.toHexString());
  await Promise.all([
    db.collection("progress").deleteMany({ userId: id }),
    db.collection("tutor_threads").deleteMany({ userId: id }),
    db.collection("xp_events").deleteMany({ userId: id }),
    db.collection("achievements").deleteMany({ userId: id }),
    db.collection("reviews").deleteMany({ userId: id }),
    db.collection("review_events").deleteMany({ userId: id }),
    db.collection("password_resets").deleteMany({ userId: id }),
    db.collection("email_verifications").deleteMany({ userId: id }),
    db.collection("email_queue").deleteMany({ recipientHash: hmac("email-queue-recipient", user.email.toLowerCase()) }),
    db.collection("pending_2fa").deleteMany({ userId: id }),
    deleteTutorUsage(db, id.toHexString()),
    db.collection("tutor_spend").deleteMany({ key: { $regex: `^u:${id.toHexString()}:` } }),
    db.collection("tutor_reservations").deleteMany({ userId: id.toHexString() }),
  ]);
}
