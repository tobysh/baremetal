"use server";

import { redirect } from "next/navigation";
import { z } from "zod";
import { getDb } from "@/lib/db";
import {
  clearPending2fa,
  createSession,
  destroySession,
  getDummyHash,
  getPending2fa,
  hashPassword,
  revokeAllSessions,
  safeNext,
  startPending2fa,
  verifyPassword,
} from "@/lib/auth";
import { decrypt, randomToken, sha256 } from "@/lib/crypto";
import { verifyCaptcha } from "@/lib/captcha";
import { sendEmail } from "@/lib/email";
import { fieldErrors, formValues, type FormState } from "@/lib/forms";
import { usernameProblem } from "@/lib/gamification";
import { EXPERIENCE_LEVELS, TERMS_VERSION, type UserDoc } from "@/lib/models";
import { checkPassword } from "@/lib/password";
import { authRateLimit, clearLoginFailures, formatWait, logAuthEvent, loginGuard, recordLoginFailure, requestContext } from "@/lib/security";
import { hashBackupCode, verifyTotp } from "@/lib/totp";
import { absoluteUrl } from "@/lib/site";

const email = z.email("Enter a valid email address, like you@example.com").max(254).transform((e) => e.trim().toLowerCase());
const TOO_MANY = "Too many attempts from your network. Please wait 15 minutes and try again.";
const username = z
  .string()
  .trim()
  .toLowerCase()
  .superRefine((v, ctx) => {
    const problem = usernameProblem(v);
    if (problem) ctx.addIssue({ code: "custom", message: problem });
  });

/** Which unique index a duplicate-key error hit. */
const duplicateField = (e: unknown): string | null =>
  (e as { code?: number }).code === 11000 ? Object.keys((e as { keyPattern?: object }).keyPattern ?? {})[0] ?? "email" : null;

// ---------- Sign up ----------

const signupSchema = z
  .object({
    name: z.string().trim().min(1, "Tell us what to call you").max(80, "Keep it under 80 characters"),
    username,
    email,
    password: z.string().min(1, "Choose a password"),
    confirm: z.string(),
    experience: z.enum(Object.keys(EXPERIENCE_LEVELS) as [keyof typeof EXPERIENCE_LEVELS], { message: "Pick the option closest to you" }),
    terms: z.literal("on", { message: "You need to accept the Terms and Privacy Notice to create an account" }),
    age: z.literal("on", { message: "You must be 16 or older to create an account" }),
  })
  .refine((d) => d.password === d.confirm, { path: ["confirm"], message: "Passwords don't match" });

export async function signup(_prev: FormState, formData: FormData): Promise<FormState> {
  const values = formValues(formData, ["name", "username", "email", "experience"]);
  if (!(await authRateLimit("signup", 5))) return { message: TOO_MANY, values };
  if (!(await verifyCaptcha(formData.get("captcha") as string | null))) {
    await logAuthEvent("captcha_failed", { reason: "signup" });
    return { message: "Please complete the security check.", values, captchaRequired: true };
  }

  const parsed = signupSchema.safeParse(Object.fromEntries(formData));
  if (!parsed.success) return { errors: fieldErrors(parsed.error), values, captchaRequired: true };
  const d = parsed.data;

  const pwError = await checkPassword(d.password, [d.email, d.name, d.username]);
  if (pwError) return { errors: { password: [pwError] }, values, captchaRequired: true };

  const db = await getDb();
  const now = new Date();
  try {
    const res = await db.collection<Omit<UserDoc, "_id">>("users").insertOne({
      email: d.email,
      name: d.name,
      username: d.username,
      xp: 0,
      profilePublic: true,
      passwordHash: await hashPassword(d.password),
      experience: d.experience,
      role: "user",
      createdAt: now,
      termsAcceptedAt: now,
      termsVersion: TERMS_VERSION,
      sessionVersion: 0,
      passwordChangedAt: now,
    });
    await logAuthEvent("signup", { email: d.email, userId: res.insertedId.toHexString() });
    const { userAgent } = await requestContext();
    await createSession({ _id: res.insertedId, sessionVersion: 0 }, userAgent);
  } catch (e) {
    const dup = duplicateField(e);
    if (dup === "username") return { errors: { username: ["That username is taken — try another"] }, values, captchaRequired: true };
    if (dup) return { errors: { email: ["An account with this email already exists. Try signing in instead."] }, values, captchaRequired: true };
    throw e;
  }
  redirect("/learn?welcome=1");
}

// ---------- Sign in ----------

const loginSchema = z.object({ email, password: z.string().min(1, "Enter your password").max(1024) });

export async function login(_prev: FormState, formData: FormData): Promise<FormState> {
  const values = formValues(formData, ["email"]);
  const next = safeNext(formData.get("next"));
  if (!(await authRateLimit("login", 20))) return { message: TOO_MANY, values, captchaRequired: true };

  const parsed = loginSchema.safeParse(Object.fromEntries(formData));
  if (!parsed.success) return { errors: fieldErrors(parsed.error), values };
  const { email: addr, password } = parsed.data;
  const { ip, userAgent } = await requestContext();

  const guard = await loginGuard(addr, ip);
  if (guard.locked) {
    await logAuthEvent("login_locked", { email: addr });
    return {
      message: `Too many failed sign-in attempts. For your security, sign-in is paused for ${formatWait(guard.retryAfterSeconds)}. You can also reset your password.`,
      values,
      captchaRequired: true,
    };
  }
  if (guard.captchaRequired && !(await verifyCaptcha(formData.get("captcha") as string | null))) {
    await logAuthEvent("captcha_failed", { email: addr, reason: "login" });
    return { message: "Please complete the security check, then sign in again.", values, captchaRequired: true };
  }

  const db = await getDb();
  const user = await db.collection<UserDoc>("users").findOne({ email: addr });
  // Always run scrypt so response time doesn't reveal whether the email exists.
  const valid = await verifyPassword(password, user?.passwordHash ?? (await getDummyHash()));
  if (!user || !valid) {
    await recordLoginFailure(addr, ip);
    await logAuthEvent("login_failed", { email: addr, reason: user ? "bad_password" : "unknown_email" });
    const after = await loginGuard(addr, ip);
    return { message: "Incorrect email or password.", values, captchaRequired: after.captchaRequired };
  }

  await clearLoginFailures(addr, ip);
  if (user.twoFactor?.enabled) {
    await startPending2fa(user._id, next);
    redirect("/login/verify");
  }
  await createSession(user, userAgent);
  await logAuthEvent("login_success", { email: addr, userId: user._id.toHexString() });
  redirect(next);
}

// ---------- Second factor ----------

const MAX_OTP_ATTEMPTS = 5;

export async function verifySecondFactor(_prev: FormState, formData: FormData): Promise<FormState> {
  if (!(await authRateLimit("verify-otp", 10))) return { message: TOO_MANY };
  const pending = await getPending2fa();
  if (!pending) redirect("/login?expired=1");

  const code = String(formData.get("code") ?? "").trim();
  if (!code) return { errors: { code: ["Enter the 6-digit code from your authenticator app, or a backup code"] } };

  const db = await getDb();
  const users = db.collection<UserDoc>("users");
  const user = await users.findOne({ _id: pending.userId });
  if (!user?.twoFactor?.enabled) {
    await clearPending2fa();
    redirect("/login");
  }

  let ok = false;
  let usedBackup = false;
  if (/^\d{6}$/.test(code.replace(/\s/g, ""))) {
    const step = verifyTotp(decrypt(user.twoFactor.secretEnc), code, user.twoFactor.lastStep);
    if (step !== null) {
      // Conditional update: a code can only be used once, even under concurrent requests.
      const res = await users.updateOne({ _id: user._id, "twoFactor.lastStep": { $lt: step } }, { $set: { "twoFactor.lastStep": step } });
      ok = res.modifiedCount === 1;
    }
  } else {
    const res = await users.updateOne({ _id: user._id }, { $pull: { "twoFactor.backupCodeHashes": hashBackupCode(code) } });
    ok = usedBackup = res.modifiedCount === 1;
  }

  if (!ok) {
    const updated = await db
      .collection("pending_2fa")
      .findOneAndUpdate({ _id: pending._id }, { $inc: { attempts: 1 } }, { returnDocument: "after" });
    await logAuthEvent("otp_failed", { email: user.email, userId: user._id.toHexString() });
    const { ip } = await requestContext();
    await recordLoginFailure(user.email, ip);
    if ((updated?.attempts ?? MAX_OTP_ATTEMPTS) >= MAX_OTP_ATTEMPTS) {
      await clearPending2fa();
      redirect("/login?expired=1");
    }
    return { errors: { code: ["That code didn't work. Codes change every 30 seconds — try the newest one."] } };
  }

  await clearPending2fa();
  const { userAgent } = await requestContext();
  await createSession(user, userAgent);
  await logAuthEvent(usedBackup ? "backup_code_used" : "login_success", { email: user.email, userId: user._id.toHexString() });
  redirect(safeNext(pending.next));
}

// ---------- Password reset ----------

const RESET_MINUTES = 30;

export async function requestPasswordReset(_prev: FormState, formData: FormData): Promise<FormState> {
  const values = formValues(formData, ["email"]);
  if (!(await authRateLimit("forgot-password", 5))) return { message: TOO_MANY, values };
  const parsed = z.object({ email }).safeParse(Object.fromEntries(formData));
  if (!parsed.success) return { errors: fieldErrors(parsed.error), values };
  const addr = parsed.data.email;

  // Same response whether or not the account exists, so this can't be used to discover accounts.
  const done: FormState = { ok: true, message: `If an account exists for ${addr}, we've sent a link to reset your password. It expires in ${RESET_MINUTES} minutes.` };

  const { ip } = await requestContext();
  const { rateLimit } = await import("@/lib/rate-limit");
  if (!(await rateLimit(`reset-email:${sha256(addr)}`, 3, 3600))) return done;

  const db = await getDb();
  const user = await db.collection<UserDoc>("users").findOne({ email: addr });
  await logAuthEvent("password_reset_requested", { email: addr, reason: user ? undefined : "unknown_email" });
  if (!user) return done;

  const token = randomToken(32);
  await db.collection("password_resets").deleteMany({ userId: user._id });
  await db.collection("password_resets").insertOne({
    tokenHash: sha256(token),
    userId: user._id,
    requestedIp: ip,
    expiresAt: new Date(Date.now() + RESET_MINUTES * 60_000),
  });
  // Delivery failures are logged, never surfaced: a different response for
  // registered emails would reveal which accounts exist.
  await sendEmail(
    user.email,
    "Reset your Baremetal password",
    `Hi ${user.name},\n\nSomeone (hopefully you) asked to reset your Baremetal password. Use this link within ${RESET_MINUTES} minutes:\n\n${absoluteUrl(`/reset-password?token=${token}`)}\n\nIf you didn't ask for this, you can ignore this email — your password won't change.\n`,
  ).catch((e) => console.error("[auth] password reset email failed", e));
  return done;
}

const resetSchema = z
  .object({ token: z.string().min(20).max(100), password: z.string().min(1, "Choose a new password"), confirm: z.string() })
  .refine((d) => d.password === d.confirm, { path: ["confirm"], message: "Passwords don't match" });

export async function resetPassword(_prev: FormState, formData: FormData): Promise<FormState> {
  if (!(await authRateLimit("reset-password", 10))) return { message: TOO_MANY };
  const parsed = resetSchema.safeParse(Object.fromEntries(formData));
  if (!parsed.success) return { errors: fieldErrors(parsed.error) };
  const { token, password } = parsed.data;

  const db = await getDb();
  const reset = await db.collection("password_resets").findOne({ tokenHash: sha256(token), expiresAt: { $gt: new Date() } });
  if (!reset) return { message: "This reset link has expired or was already used. Request a new one." };
  const user = await db.collection<UserDoc>("users").findOne({ _id: reset.userId });
  if (!user) return { message: "This reset link has expired or was already used. Request a new one." };

  const pwError = await checkPassword(password, [user.email, user.name]);
  if (pwError) return { errors: { password: [pwError] } };
  if (await verifyPassword(password, user.passwordHash)) return { errors: { password: ["Choose a password you haven't used here before."] } };

  // Single use: delete first so a concurrent request can't reuse the token.
  const del = await db.collection("password_resets").deleteOne({ _id: reset._id });
  if (del.deletedCount !== 1) return { message: "This reset link has expired or was already used. Request a new one." };

  await db.collection<UserDoc>("users").updateOne({ _id: user._id }, { $set: { passwordHash: await hashPassword(password), passwordChangedAt: new Date() } });
  await revokeAllSessions(user._id);
  const { ip } = await requestContext();
  await clearLoginFailures(user.email, ip);
  await logAuthEvent("password_reset", { email: user.email, userId: user._id.toHexString() });
  await sendEmail(user.email, "Your Baremetal password was changed", `Hi ${user.name},\n\nYour password was just reset and all devices were signed out. If this wasn't you, reset your password again immediately and contact us.\n`).catch((e) =>
    console.error("[auth] password-changed email failed", e),
  );
  redirect("/login?reset=1");
}

export async function logout() {
  await destroySession();
  redirect("/");
}
