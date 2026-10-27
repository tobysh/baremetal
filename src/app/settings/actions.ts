"use server";

import { redirect } from "next/navigation";
import { revalidatePath } from "next/cache";
import { z } from "zod";
import QRCode from "qrcode";
import { getDb } from "@/lib/db";
import { destroySession, getUserDoc, hashPassword, revokeAllSessions, verifyPassword } from "@/lib/auth";
import { decrypt, encrypt } from "@/lib/crypto";
import { fieldErrors, formValues, type FormState } from "@/lib/forms";
import { usernameProblem } from "@/lib/gamification";
import { EXPERIENCE_LEVELS, type UserDoc } from "@/lib/models";
import { checkPassword } from "@/lib/password";
import { authRateLimit, logAuthEvent, requestContext } from "@/lib/security";
import { hashBackupCode, newBackupCodes, newTotpSecret, otpauthUrl, verifyTotp } from "@/lib/totp";
import { SITE } from "@/lib/site";

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
  const res = await (await users()).updateOne({ _id: user._id }, { $pull: { "twoFactor.backupCodeHashes": hashBackupCode(code) } });
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
export async function beginTwoFactorSetup(): Promise<FormState> {
  const user = await mustUser();
  if (user.twoFactor?.enabled) return { message: "Two-factor authentication is already on." };
  const secret = newTotpSecret();
  await (await users()).updateOne(
    { _id: user._id },
    { $set: { twoFactor: { enabled: false, secretEnc: encrypt(secret), backupCodeHashes: [], lastStep: -1 } } },
  );
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
  await (await users()).updateOne(
    { _id: user._id },
    {
      $set: {
        "twoFactor.enabled": true,
        "twoFactor.lastStep": step,
        "twoFactor.backupCodeHashes": codes.map(hashBackupCode),
        "twoFactor.enabledAt": new Date(),
      },
    },
  );
  const { userAgent } = await requestContext();
  await revokeAllSessions(user._id, { userAgent });
  await logAuthEvent("2fa_enabled", { email: user.email, userId: user._id.toHexString() });
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
  if (!(await authRateLimit("verify-otp", 10))) return { message: "Too many attempts. Please wait 15 minutes." };
  if (!(await verifyPassword(String(formData.get("password") ?? ""), user.passwordHash))) return { errors: { password: ["Incorrect password"] } };
  if (!(await checkSecondFactor(user, String(formData.get("code") ?? "")))) return { errors: { code: ["That code didn't work"] } };
  await (await users()).updateOne({ _id: user._id }, { $unset: { twoFactor: "" } });
  const { userAgent } = await requestContext();
  await revokeAllSessions(user._id, { userAgent });
  await logAuthEvent("2fa_disabled", { email: user.email, userId: user._id.toHexString() });
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
    db.collection("sessions").deleteMany({ userId: user._id }),
    db.collection("password_resets").deleteMany({ userId: user._id }),
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
