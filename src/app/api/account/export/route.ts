import { NextResponse } from "next/server";
import { getUserDoc } from "@/lib/auth";
import { hmac } from "@/lib/crypto";
import { exportTutorUsage } from "@/lib/tutor-budget";
import { getDb } from "@/lib/db";
import { jsonError } from "@/lib/http";
import { rateLimit } from "@/lib/rate-limit";

/** GDPR Article 15/20: everything we hold about the signed-in user, as JSON. */
export async function GET() {
  const user = await getUserDoc();
  if (!user) return jsonError(401, "Sign in to export your data.");
  if (!(await rateLimit(`export:${user._id.toHexString()}`, 5, 3600))) return jsonError(429, "Too many exports — try again later.");
  const db = await getDb();
  const [progress, tutor, sessions, securityLog, xpEvents, achievements, reviews, reviewEvents, pendingEmail, verification, tutorSpend, adminActions, pending2fa, tutorUsage, tutorReservations] = await Promise.all([
    db.collection("progress").find({ userId: user._id }, { projection: { _id: 0, userId: 0 } }).toArray(),
    db.collection("tutor_threads").find({ userId: user._id }, { projection: { _id: 0, userId: 0 } }).toArray(),
    db.collection("sessions").find({ userId: user._id }, { projection: { _id: 0, createdAt: 1, expiresAt: 1, userAgent: 1 } }).toArray(),
    db
      .collection("auth_events")
      .find({ $or: [{ userId: user._id.toHexString() }, { email: user.email }] }, { projection: { _id: 0 } })
      .toArray(),
    db.collection("xp_events").find({ userId: user._id }, { projection: { _id: 0, userId: 0 } }).toArray(),
    db.collection("achievements").find({ userId: user._id }, { projection: { _id: 0, userId: 0 } }).toArray(),
    db.collection("reviews").find({ userId: user._id }, { projection: { _id: 0, userId: 0 } }).toArray(),
    db.collection("review_events").find({ userId: user._id }, { projection: { _id: 0, userId: 0, retryOf: 0 } }).toArray(),
    db.collection("email_queue").find({ recipientHash: hmac("email-queue-recipient", user.email.toLowerCase()) }, { projection: { _id: 0, createdAt: 1, nextAttempt: 1, expiresAt: 1, attempts: 1 } }).toArray(),
    db.collection("email_verifications").find({ userId: user._id }, { projection: { _id: 0, expiresAt: 1 } }).toArray(),
    db.collection("tutor_spend").find({ key: { $regex: `^u:${user._id.toHexString()}:` } }, { projection: { _id: 0, key: 1, micro: 1, createdAt: 1 } }).toArray(),
    db.collection("admin_audit").find({ targetId: user._id.toHexString() }, { projection: { _id: 0, action: 1, note: 1, ts: 1 } }).toArray(),
    db.collection("pending_2fa").find({ userId: user._id }, { projection: { _id: 0, expiresAt: 1, attempts: 1, next: 1 } }).toArray(),
    exportTutorUsage(db, user._id.toHexString()),
    db.collection("tutor_reservations").find({ userId: user._id.toHexString() }, { projection: { userId: 0 } }).toArray(),
  ]);
  const data = {
    exportedAt: new Date().toISOString(),
    account: {
      email: user.email,
      emailVerified: user.emailVerified ?? null,
      name: user.name,
      username: user.username ?? null,
      xp: user.xp ?? 0,
      profilePublic: user.profilePublic !== false,
      timezone: user.timezone ?? null,
      weeklyGoal: user.weeklyGoal ?? null,
      experience: user.experience,
      role: user.role,
      createdAt: user.createdAt,
      termsAcceptedAt: user.termsAcceptedAt,
      termsVersion: user.termsVersion,
      suspension: user.suspension ? { kind: user.suspension.kind, at: user.suspension.at, until: user.suspension.until ?? null, reason: user.suspension.reason ?? null } : null,
      passwordChangedAt: user.passwordChangedAt,
      twoFactorEnabled: !!user.twoFactor?.enabled,
      unusedBackupCodes: user.twoFactor?.backupCodeHashes.length ?? 0,
      note: "Your password, 2FA secret and backup codes are stored only in hashed/encrypted form and are not included. Tutor usage records written before this export existed (they carry no identity), shared tutor replies, rate-limit counters, sign-in failure counters and analytics events are not linked to your account (they hold keyed hashes or no identity), so they cannot be exported.",
    },
    progress,
    xpLedger: xpEvents,
    achievements,
    reviewSchedule: reviews,
    reviewHistory: reviewEvents,
    tutorConversations: tutor,
    activeSessions: sessions,
    securityLog,
    pendingEmailDelivery: pendingEmail,
    pendingEmailVerification: verification,
    tutorSpend,
    tutorUsage,
    tutorReservations,
    administratorActions: adminActions,
    pendingTwoFactorSignIns: pending2fa,
  };
  return new NextResponse(JSON.stringify(data, null, 2), {
    headers: {
      "Content-Type": "application/json; charset=utf-8",
      "Content-Disposition": `attachment; filename="baremetal-data-${new Date().toISOString().slice(0, 10)}.json"`,
      "Cache-Control": "no-store",
    },
  });
}
