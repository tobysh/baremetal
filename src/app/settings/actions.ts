"use server";

import { redirect } from "next/navigation";
import { revalidatePath } from "next/cache";
import { z } from "zod";
import QRCode from "qrcode";
import { getDb } from "@/lib/db";
import { destroySession, getUserDoc, hashPassword, revokeAllSessions, verifyPassword } from "@/lib/auth";
import { decrypt, encrypt, hmac } from "@/lib/crypto";
import { queueEmail, scheduleEmailDelivery } from "@/lib/email";
import { fieldErrors, formValues, type FormState } from "@/lib/forms";
import { usernameProblem } from "@/lib/gamification";
import { EXPERIENCE_LEVELS, type UserDoc } from "@/lib/models";
import { checkPassword } from "@/lib/password";
import { authRateLimit, logAuthEvent, requestContext } from "@/lib/security";
import { hashBackupCode, legacyBackupCodeHash, newBackupCodes, newTotpSecret, otpauthUrl, verifyTotp } from "@/lib/totp";
import { SITE } from "@/lib/site";
import { validTimezone, WEEKLY_GOALS } from "@/lib/rhythm";

async function mustUser(): Promise<UserDoc> {
  const user = await getUserDoc();
  if (!user) redirect("/login?next=/settings");
  return user;
}

const users = async () => (await getDb()).collection<UserDoc>("users");

/** Re-check a second factor for sensitive changes when 2FA is on. */
async function checkSecondFactor(user: UserDoc, code: string): Promise<boolean> {
  if (!user.twoFactor?.enabled) return true;
  const step = verifyTotp(decrypt(user.twoFactor.secretEnc), code, user.twoFactor.lastStep);
  if (step !== null) {
    const res = await (await users()).updateOne({ _id: user._id, "twoFactor.lastStep": { $lt: step } }, { $set: { "twoFactor.lastStep": step } });
    return res.modifiedCount === 1;
  }
  const res = await (await users()).updateOne({ _id: user._id }, { $pull: { "twoFactor.backupCodeHashes": { $in: [hashBackupCode(code), legacyBackupCodeHash(code)] } } });
  return res.modifiedCount === 1;
}

// ---------- Profile ----------

const profileSchema = z.object({
  name: z.string().trim().min(1, "Tell us what to call you").max(80, "Keep it under 80 characters"),
  username: z
    .string()
    .trim()
    .toLowerCase()
    .superRefine((v, ctx) => {
      const problem = usernameProblem(v);
      if (problem) ctx.addIssue({ code: "custom", message: problem });
    }),
  profilePublic: z.literal("on").optional(),
  experience: z.enum(Object.keys(EXPERIENCE_LEVELS) as [keyof typeof EXPERIENCE_LEVELS], { message: "Pick the option closest to you" }),
});

export async function updateProfile(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  const values = formValues(formData, ["name", "username", "experience", "profilePublic"]);
  const parsed = profileSchema.safeParse(Object.fromEntries(formData));
  if (!parsed.success) return { errors: fieldErrors(parsed.error), values };
  const { profilePublic, ...rest } = parsed.data;
  try {
    await (await users()).updateOne({ _id: user._id }, { $set: { ...rest, profilePublic: profilePublic === "on" } });
  } catch (e) {
    if ((e as { code?: number }).code === 11000) return { errors: { username: ["That username is taken — try another"] }, values };
    throw e;
  }
  revalidatePath("/settings");
  return { ok: true, message: "Profile saved.", values: { ...values, username: rest.username } };
}

// ---------- Rhythm (weekly goal and time zone) ----------

export async function updateRhythm(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  const goal = Number(formData.get("weeklyGoal"));
  const timezone = String(formData.get("timezone") ?? "");
  if (goal !== 0 && !(WEEKLY_GOALS as readonly number[]).includes(goal)) return { errors: { weeklyGoal: ["Pick one of the options"] } };
  if (!validTimezone(timezone)) return { errors: { timezone: ["Pick a time zone from the list"] } };
  await (await users()).updateOne({ _id: user._id }, { $set: { weeklyGoal: goal, timezone } });
  revalidatePath("/settings");
  revalidatePath("/dashboard");
  return { ok: true, message: "Saved." };
}

/** Called silently by the dashboard the first time we see a learner, so weeks start on their own Monday. */
export async function rememberTimezone(timezone: string): Promise<void> {
  const user = await getUserDoc();
  if (!user || user.timezone || !validTimezone(timezone)) return;
  await (await users()).updateOne({ _id: user._id, timezone: { $exists: false } }, { $set: { timezone } });
}

// ---------- Password ----------

const passwordSchema = z
  .object({ current: z.string().min(1, "Enter your current password"), password: z.string().min(1, "Choose a new password"), confirm: z.string() })
  .refine((d) => d.password === d.confirm, { path: ["confirm"], message: "Passwords don't match" });

export async function changePassword(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  if (!(await authRateLimit("change-password", 5))) return { message: "Too many attempts. Please wait 15 minutes." };
  const parsed = passwordSchema.safeParse(Object.fromEntries(formData));
  if (!parsed.success) return { errors: fieldErrors(parsed.error) };
  if (!(await verifyPassword(parsed.data.current, user.passwordHash))) {
    await logAuthEvent("login_failed", { email: user.email, userId: user._id.toHexString(), reason: "change_password_wrong_current" });
    return { errors: { current: ["That isn't your current password"] } };
  }
  const pwError = await checkPassword(parsed.data.password, [user.email, user.name]);
  if (pwError) return { errors: { password: [pwError] } };
  await (await users()).updateOne({ _id: user._id }, { $set: { passwordHash: await hashPassword(parsed.data.password), passwordChangedAt: new Date() } });
  const { userAgent } = await requestContext();
  await revokeAllSessions(user._id, { userAgent });
  await logAuthEvent("password_changed", { email: user.email, userId: user._id.toHexString() });
  return { ok: true, message: "Password changed. You've been signed out on all other devices." };
}

// ---------- Two-factor authentication ----------

/** Step 1: generate a secret (stored but not yet enabled) and return a QR code. */
export async function beginTwoFactorSetup(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  if (user.twoFactor?.enabled) return { message: "Two-factor authentication is already on." };
  if (!(await authRateLimit("begin-2fa", 5))) return { message: "Too many attempts. Please wait 15 minutes." };
  if (!(await verifyPassword(String(formData.get("password") ?? ""), user.passwordHash))) return { errors: { password: ["Incorrect password"] } };
  const secret = newTotpSecret();
  const started = await (await users()).updateOne(
    { _id: user._id, "twoFactor.enabled": { $ne: true } },
    { $set: { twoFactor: { enabled: false, secretEnc: encrypt(secret), backupCodeHashes: [], lastStep: -1 } } },
  );
  if (!started.matchedCount) return { message: "Two-factor authentication is already on." };
  const url = otpauthUrl(secret, user.email, SITE.name);
  const qrSvg = await QRCode.toString(url, { type: "svg", margin: 1, errorCorrectionLevel: "M" });
  return { ok: true, data: { qrSvg, secret: secret.match(/.{1,4}/g)?.join(" ") } };
}

/** Step 2: confirm with a code from the app, enable 2FA, and show backup codes once. */
export async function confirmTwoFactor(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  if (!(await authRateLimit("verify-otp", 10))) return { message: "Too many attempts. Please wait 15 minutes." };
  if (!user.twoFactor || user.twoFactor.enabled) return { message: "Start setup again." };
  const code = String(formData.get("code") ?? "");
  const step = verifyTotp(decrypt(user.twoFactor.secretEnc), code);
  if (step === null) {
    await logAuthEvent("otp_failed", { email: user.email, userId: user._id.toHexString(), reason: "setup" });
    return { errors: { code: ["That code didn't match. Check your phone's clock is set automatically and use the newest code."] } };
  }
  const codes = newBackupCodes();
  const enabled = await (await users()).updateOne(
    { _id: user._id, "twoFactor.enabled": false, "twoFactor.secretEnc": user.twoFactor.secretEnc },
    {
      $set: {
        "twoFactor.enabled": true,
        "twoFactor.lastStep": step,
        "twoFactor.backupCodeHashes": codes.map(hashBackupCode),
        "twoFactor.enabledAt": new Date(),
      },
    },
  );
  if (!enabled.modifiedCount) return { message: "Setup changed. Start again." };
  const { userAgent } = await requestContext();
  await revokeAllSessions(user._id, { userAgent });
  await logAuthEvent("2fa_enabled", { email: user.email, userId: user._id.toHexString() });
  await queueEmail(user.email, "Two-factor authentication enabled", "Two-factor authentication was enabled on your Baremetal account. If this was not you, reset your password and contact us.");
  scheduleEmailDelivery();
  return { ok: true, message: "Two-factor authentication is on.", data: { backupCodes: codes } };
}

export async function regenerateBackupCodes(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  if (!user.twoFactor?.enabled) return { message: "Turn on two-factor authentication first." };
  if (!(await authRateLimit("verify-otp", 10))) return { message: "Too many attempts. Please wait 15 minutes." };
  if (!(await checkSecondFactor(user, String(formData.get("code") ?? "")))) return { errors: { code: ["That code didn't work"] } };
  const codes = newBackupCodes();
  await (await users()).updateOne({ _id: user._id }, { $set: { "twoFactor.backupCodeHashes": codes.map(hashBackupCode) } });
  return { ok: true, message: "New backup codes created. Your old codes no longer work.", data: { backupCodes: codes } };
}

export async function disableTwoFactor(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  if (user.role === "admin") return { message: "Admin accounts must keep two-factor authentication on." };
  if (!user.twoFactor?.enabled) return { message: "Two-factor authentication is already off." };
  if (!(await authRateLimit("verify-otp", 10))) return { message: "Too many attempts. Please wait 15 minutes." };
  if (!(await verifyPassword(String(formData.get("password") ?? ""), user.passwordHash))) return { errors: { password: ["Incorrect password"] } };
  if (!(await checkSecondFactor(user, String(formData.get("code") ?? "")))) return { errors: { code: ["That code didn't work"] } };
  await (await users()).updateOne({ _id: user._id }, { $unset: { twoFactor: "" } });
  const { userAgent } = await requestContext();
  await revokeAllSessions(user._id, { userAgent });
  await logAuthEvent("2fa_disabled", { email: user.email, userId: user._id.toHexString() });
  await queueEmail(user.email, "Two-factor authentication disabled", "Two-factor authentication was disabled on your Baremetal account. If this was not you, reset your password and contact us.");
  scheduleEmailDelivery();
  return { ok: true, message: "Two-factor authentication is off." };
}

// ---------- Delete account (GDPR erasure) ----------

export async function deleteAccount(_prev: FormState, formData: FormData): Promise<FormState> {
  const user = await mustUser();
  if (!(await authRateLimit("delete-account", 5))) return { message: "Too many attempts. Please wait 15 minutes." };
  if (String(formData.get("confirm") ?? "").trim().toLowerCase() !== "delete my account")
    return { errors: { confirm: ['Type "delete my account" to confirm'] } };
  if (!(await verifyPassword(String(formData.get("password") ?? ""), user.passwordHash))) return { errors: { password: ["Incorrect password"] } };
  if (!(await checkSecondFactor(user, String(formData.get("code") ?? "")))) return { errors: { code: ["That code didn't work"] } };

  const db = await getDb();
  await Promise.all([
    db.collection("progress").deleteMany({ userId: user._id }),
    db.collection("tutor_threads").deleteMany({ userId: user._id }),
    db.collection("xp_events").deleteMany({ userId: user._id }),
    db.collection("achievements").deleteMany({ userId: user._id }),
    db.collection("reviews").deleteMany({ userId: user._id }),
    db.collection("sessions").deleteMany({ userId: user._id }),
    db.collection("password_resets").deleteMany({ userId: user._id }),
    db.collection("email_verifications").deleteMany({ userId: user._id }),
    db.collection("email_queue").deleteMany({ recipientHash: hmac("email-queue-recipient", user.email.toLowerCase()) }),
    db.collection("pending_2fa").deleteMany({ userId: user._id }),
  ]);
  await db.collection("users").deleteOne({ _id: user._id });
  // Security log entries are kept for their 90-day retention (legitimate interest), not linked to an account any more.
  await logAuthEvent("account_deleted", { userId: user._id.toHexString() });
  await destroySession();
  redirect("/?deleted=1");
}

// ---------- Tutor history ----------

export async function clearTutorHistory(): Promise<FormState> {
  const user = await mustUser();
  await (await getDb()).collection("tutor_threads").deleteMany({ userId: user._id });
  return { ok: true, message: "All tutor conversations deleted." };
}
