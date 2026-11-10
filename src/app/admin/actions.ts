"use server";

import { redirect } from "next/navigation";
import { revalidatePath } from "next/cache";
import { ObjectId } from "mongodb";
import { z } from "zod";
import { deleteUserData, issueEmailVerification, issuePasswordReset } from "@/lib/account-ops";
import { logAdminAction, type AdminAction } from "@/lib/admin";
import { getDb } from "@/lib/db";
import { hashPassword, requireAdmin, revokeAllSessions } from "@/lib/auth";
import { randomToken } from "@/lib/crypto";
import { scheduleEmailDelivery } from "@/lib/email";
import { fieldErrors, formValues, type FormState } from "@/lib/forms";
import { usernameProblem } from "@/lib/gamification";
import { rateLimit } from "@/lib/rate-limit";
import type { UserDoc } from "@/lib/models";

// Every action re-checks the admin gate (role + 2FA) on the server; the UI is never trusted.
// Admin accounts cannot be changed here: promoting, demoting and acting on admins stays on the command line.

type Target = { admin: Awaited<ReturnType<typeof requireAdmin>>; user: UserDoc };
type Gate = { ok: true; target: Target } | { ok: false; state: FormState };

async function gate(formData: FormData): Promise<Gate> {
  const admin = await requireAdmin();
  if (!(await rateLimit(`admin-action:${admin.id}`, 120, 3600))) return { ok: false, state: { message: "Too many admin actions this hour. Wait a while and try again." } };
  const id = String(formData.get("userId") ?? "");
  if (!/^[0-9a-f]{24}$/i.test(id)) return { ok: false, state: { message: "Unknown account." } };
  const user = await (await getDb()).collection<UserDoc>("users").findOne({ _id: new ObjectId(id) });
  if (!user) return { ok: false, state: { message: "That account no longer exists." } };
  if (user.role === "admin" || user._id.toHexString() === admin.id)
    return { ok: false, state: { message: "Admin accounts can only be changed from the command line." } };
  return { ok: true, target: { admin, user } };
}

const reasonOf = (formData: FormData) => String(formData.get("reason") ?? "").trim().slice(0, 200) || undefined;
const done = (t: Target, action: AdminAction, message: string, note?: string): Promise<FormState> =>
  logAdminAction(t.admin.id, action, t.user._id.toHexString(), note).then(() => {
    revalidatePath(`/admin/users/${t.user._id.toHexString()}`);
    revalidatePath("/admin/users");
    return { ok: true, message };
  });

// ---------- Create ----------

const createSchema = z.object({
  name: z.string().trim().min(1, "Enter a name").max(80),
  email: z.email("Enter a valid email address").max(254).transform((e) => e.trim().toLowerCase()),
  username: z.string().trim().toLowerCase().superRefine((v, ctx) => {
    const problem = usernameProblem(v);
    if (problem) ctx.addIssue({ code: "custom", message: problem });
  }),
});

/** The new user chooses their own password through the verification link, so nobody else ever knows it. */
export async function createAccount(_prev: FormState, formData: FormData): Promise<FormState> {
  const admin = await requireAdmin();
  const values = formValues(formData, ["name", "email", "username"]);
  if (!(await rateLimit(`admin-action:${admin.id}`, 120, 3600))) return { message: "Too many admin actions this hour.", values };
  const parsed = createSchema.safeParse(Object.fromEntries(formData));
  if (!parsed.success) return { errors: fieldErrors(parsed.error), values };
  const d = parsed.data;
  const now = new Date();
  const db = await getDb();
  let id: ObjectId;
  try {
    id = (await db.collection<Omit<UserDoc, "_id">>("users").insertOne({
      email: d.email, name: d.name, username: d.username, xp: 0, profilePublic: true,
      passwordHash: await hashPassword(randomToken(32)), experience: "some", role: "user",
      createdAt: now, termsAcceptedAt: new Date(0), termsVersion: "pending", sessionVersion: 0, passwordChangedAt: now, emailVerified: false,
    })).insertedId;
  } catch (e) {
    if ((e as { code?: number }).code === 11000) {
      const field = Object.keys((e as { keyPattern?: object }).keyPattern ?? {})[0] ?? "email";
      return { errors: { [field === "username" ? "username" : "email"]: [`That ${field} is already taken.`] }, values };
    }
    throw e;
  }
  await issueEmailVerification({ _id: id, email: d.email }, "An administrator created a Baremetal account for this address. Use this link within 24 hours to confirm your email and choose your password:");
  scheduleEmailDelivery();
  await logAdminAction(admin.id, "user.create", id.toHexString());
  revalidatePath("/admin/users");
  redirect(`/admin/users/${id.toHexString()}`);
}

// ---------- Per-account actions ----------

export async function sendResetLink(_prev: FormState, formData: FormData): Promise<FormState> {
  const g = await gate(formData);
  if (!g.ok) return g.state;
  const { user } = g.target;
  if (user.suspension?.kind === "banned") return { message: "Lift the ban first." };
  await issuePasswordReset(user);
  scheduleEmailDelivery();
  return done(g.target, "user.reset_link", `Reset link emailed to ${user.email}. It expires in 30 minutes.`);
}

export async function resendVerification(_prev: FormState, formData: FormData): Promise<FormState> {
  const g = await gate(formData);
  if (!g.ok) return g.state;
  const { user } = g.target;
  if (user.emailVerified !== false) return { message: "This email is already verified." };
  await issueEmailVerification(user);
  scheduleEmailDelivery();
  return done(g.target, "user.resend_verification", `Verification email sent to ${user.email}.`);
}

const LOCK_HOURS: Record<string, number | null> = { "1h": 1, "24h": 24, "7d": 168, "30d": 720, forever: null };

export async function lockAccount(_prev: FormState, formData: FormData): Promise<FormState> {
  const g = await gate(formData);
  if (!g.ok) return g.state;
  const duration = String(formData.get("duration") ?? "");
  if (!Object.hasOwn(LOCK_HOURS, duration)) return { errors: { duration: ["Choose how long to lock the account."] } };
  const hours = LOCK_HOURS[duration];
  const reason = reasonOf(formData);
  const now = new Date();
  const { user } = g.target;
  await (await getDb()).collection<UserDoc>("users").updateOne({ _id: user._id }, { $set: { suspension: { kind: "locked", at: now, ...(reason ? { reason } : {}), ...(hours ? { until: new Date(now.getTime() + hours * 3_600_000) } : {}) } } });
  await revokeAllSessions(user._id);
  return done(g.target, "user.lock", hours ? `Locked for ${duration}. They were signed out everywhere.` : "Locked until you unlock it. They were signed out everywhere.", `${duration}${reason ? `: ${reason}` : ""}`);
}

export async function banAccount(_prev: FormState, formData: FormData): Promise<FormState> {
  const g = await gate(formData);
  if (!g.ok) return g.state;
  const { user } = g.target;
  if (String(formData.get("confirm") ?? "").trim().toLowerCase() !== user.email) return { errors: { confirm: ["Type the account's email address to confirm."] } };
  const reason = reasonOf(formData);
  const db = await getDb();
  // A ban also takes the profile off the leaderboard and cancels any link that could sign them back in.
  await db.collection<UserDoc>("users").updateOne({ _id: user._id }, { $set: { profilePublic: false, suspension: { kind: "banned", at: new Date(), ...(reason ? { reason } : {}) } } });
  await Promise.all([db.collection("password_resets").deleteMany({ userId: user._id }), db.collection("email_verifications").deleteMany({ userId: user._id })]);
  await revokeAllSessions(user._id);
  return done(g.target, "user.ban", "Banned. They were signed out everywhere and removed from the leaderboard.", reason);
}

export async function liftSuspension(_prev: FormState, formData: FormData): Promise<FormState> {
  const g = await gate(formData);
  if (!g.ok) return g.state;
  const { user } = g.target;
  const kind = user.suspension?.kind;
  if (!kind) return { message: "This account is not locked or banned." };
  await (await getDb()).collection<UserDoc>("users").updateOne({ _id: user._id }, { $unset: { suspension: "" } });
  return done(g.target, kind === "banned" ? "user.unban" : "user.unlock", kind === "banned" ? "Ban lifted. Their profile stays private until they turn it back on." : "Account unlocked.");
}

export async function signOutEverywhere(_prev: FormState, formData: FormData): Promise<FormState> {
  const g = await gate(formData);
  if (!g.ok) return g.state;
  await revokeAllSessions(g.target.user._id);
  return done(g.target, "user.revoke_sessions", "Signed out on every device.");
}

export async function deleteAccountAsAdmin(_prev: FormState, formData: FormData): Promise<FormState> {
  const g = await gate(formData);
  if (!g.ok) return g.state;
  const { admin, user } = g.target;
  if (String(formData.get("confirm") ?? "").trim().toLowerCase() !== user.email) return { errors: { confirm: ["Type the account's email address to confirm."] } };
  await deleteUserData(user);
  await logAdminAction(admin.id, "user.delete", user._id.toHexString(), reasonOf(formData));
  revalidatePath("/admin/users");
  redirect("/admin/users?deleted=1");
}

// ---------- Tutor cache ----------

export async function purgeTutorAnswer(formData: FormData): Promise<void> {
  const admin = await requireAdmin();
  const id = String(formData.get("id") ?? "");
  if (!/^[0-9a-f]{24}$/i.test(id) || !(await rateLimit(`admin-action:${admin.id}`, 120, 3600))) return;
  const res = await (await getDb()).collection("tutor_cache").deleteOne({ _id: new ObjectId(id) });
  if (res.deletedCount) await logAdminAction(admin.id, "tutor.purge_answer", undefined, id);
  revalidatePath("/admin");
}
