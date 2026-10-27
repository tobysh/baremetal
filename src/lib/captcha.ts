import "server-only";
import { randomBytes, randomInt } from "node:crypto";
import { getDb } from "./db";
import { hmac, safeEqual, sha256 } from "./crypto";

// Self-hosted proof-of-work CAPTCHA (same idea as ALTCHA). The browser must
// find a secret number by brute force, which costs a person ~1 second once but
// makes automated password guessing expensive. No third-party scripts or
// cookies, so it needs no consent. It slows bots down; it does not prove the
// visitor is human.

const MAX_NUMBER = 150_000;
const TTL_SECONDS = 600;

export type CaptchaChallenge = { salt: string; challenge: string; max: number; signature: string };

export function createChallenge(): CaptchaChallenge {
  const expires = Math.floor(Date.now() / 1000) + TTL_SECONDS;
  const salt = `${randomBytes(12).toString("hex")}.${expires}`;
  const challenge = sha256(`${salt}${randomInt(MAX_NUMBER)}`);
  return { salt, challenge, max: MAX_NUMBER, signature: hmac("captcha", `${salt}|${challenge}`) };
}

/** `solution` is the JSON the client widget submits. Single use. */
export async function verifyCaptcha(solution: string | null | undefined): Promise<boolean> {
  if (!solution) return false;
  let parsed: { salt?: string; challenge?: string; number?: number; signature?: string };
  try {
    parsed = JSON.parse(solution);
  } catch {
    return false;
  }
  const { salt, challenge, number, signature } = parsed;
  if (typeof salt !== "string" || typeof challenge !== "string" || typeof signature !== "string" || !Number.isInteger(number)) return false;
  if (!safeEqual(signature, hmac("captcha", `${salt}|${challenge}`))) return false;
  const expires = Number(salt.split(".")[1]);
  if (!expires || expires < Date.now() / 1000) return false;
  if (sha256(`${salt}${number}`) !== challenge) return false;
  // Reject replays of a solved challenge.
  const db = await getDb();
  try {
    await db.collection("rate_limits").insertOne({ key: `captcha:${challenge}`, count: 1, expiresAt: new Date(expires * 1000) });
  } catch {
    return false;
  }
  return true;
}
