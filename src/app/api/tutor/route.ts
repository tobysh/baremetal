import { ObjectId } from "mongodb";
import { NextResponse } from "next/server";
import { getCurrentUser, getUserDoc } from "@/lib/auth";
import { findExercise } from "@/lib/curriculum";
import { getDb } from "@/lib/db";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { hmac } from "@/lib/crypto";
import type { TutorThreadDoc } from "@/lib/models";
import { getTutorThread } from "@/lib/progress";
import { clientIp, ipLimit, rateLimit, rateLimitHit } from "@/lib/rate-limit";
import { checkBudget, estimateCostMicro, recordSpend } from "@/lib/tutor-budget";
import { TutorUnavailableError, exposesSolution, buildMessages, lessonContext, responseCacheKey, streamTutor, type TutorUsage } from "@/lib/tutor";

type Body = { exerciseId?: string; message?: string; code?: string; lastOutput?: string };

// Layered limits on model calls, so one account, one machine, or everyone
// together can't run up an unbounded bill. Answers served from the response
// cache cost nothing and don't count. All configurable in .env.local.
const env = (name: string, fallback: number) => Math.max(0, Number(process.env[name]) || fallback);
const LIMITS = {
  userPerHour: env("TUTOR_LIMIT_USER_HOUR", 20),
  userPerDay: env("TUTOR_LIMIT_USER_DAY", 100),
  ipPerHour: env("TUTOR_LIMIT_IP_HOUR", 60),
  globalPerHour: env("TUTOR_LIMIT_GLOBAL_HOUR", 2000),
  requestsPerHour: 120, // all requests, including cache hits
};
const HOUR = 3600;

function minutesUntil(d: Date) {
  return Math.max(1, Math.ceil((d.getTime() - Date.now()) / 60_000));
}

export async function POST(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const user = await getCurrentUser();
  if (!user) return jsonError(401, "Sign in to chat with the tutor.");
  const body = await readJson<Body>(req);
  if (!body || typeof body.message !== "string" || (body.code !== undefined && typeof body.code !== "string") || (body.lastOutput !== undefined && typeof body.lastOutput !== "string")) return jsonError(400, "Invalid request.");
  if (Buffer.byteLength(body.code ?? "", "utf8") > 16 * 1024 || Buffer.byteLength(body.lastOutput ?? "", "utf8") > 16 * 1024) return jsonError(400, "Code and output must each be at most 16 KB.");
  const message = body.message.trim();
  if (!body || !message || typeof body.exerciseId !== "string") return jsonError(400, "Invalid request.");
  if (message.length > 4000) return jsonError(413, "That message is a bit long — try asking about one thing at a time.");
  const found = findExercise(body.exerciseId);
  if (!found) return jsonError(404, "Unknown exercise.");
  if (!(await rateLimit(`tutor:req:${user.id}`, LIMITS.requestsPerHour, HOUR))) return jsonError(429, "Too many requests. Please slow down.");

  const db = await getDb();
  const code = body.code ?? "";
  const lastOutput = body.lastOutput ?? "";
  const lessonCtx = lessonContext(found.track, found.lesson, user.experience);
  const thread = await getTutorThread(user.id, found.exercise.id);
  const firstTurn = !thread?.messages.length;
  const cacheKey = firstTurn ? responseCacheKey({ lessonCtx, message, code, lastOutput, language: found.exercise.language }) : null;
  const cached = cacheKey ? await db.collection<{ key: string; reply: string }>("tutor_cache").findOne({ key: cacheKey }) : null;

  const saveTurn = async (reply: string) => {
    const now = new Date();
    await db.collection<TutorThreadDoc>("tutor_threads").updateOne(
      { userId: new ObjectId(user.id), lessonId: found.exercise.id },
      {
        $push: { messages: { $each: [{ role: "user", content: message, at: now }, { role: "assistant", content: reply, at: now }], $slice: -100 } },
        $set: { updatedAt: now },
      },
      { upsert: true },
    );
  };
  const recordUsage = (usage: Partial<TutorUsage> & { responseCacheHit: boolean; costMicroUsd?: number }) =>
    db.collection("tutor_usage").insertOne({ ...usage, exerciseId: found.exercise.id, ts: new Date() });

  const encoder = new TextEncoder();
  const headers: Record<string, string> = { "Content-Type": "text/plain; charset=utf-8", "Cache-Control": "no-store", "X-Tutor-Cache": cached ? "hit" : "miss" };

  if (cached) {
    await Promise.all([saveTurn(cached.reply), recordUsage({ responseCacheHit: true }), db.collection("tutor_cache").updateOne({ key: cacheKey }, { $inc: { hits: 1 } })]);
    return new Response(encoder.encode(cached.reply), { headers });
  }

  // Reserve model capacity for established accounts; cached help remains available immediately.
  const account = await getUserDoc();
  if (account?.emailVerified === false) return jsonError(403, "Verify your email address to use the tutor.");
  const age = Date.now() - (account?.createdAt.getTime() ?? Date.now());
  if (age < 10 * 60_000) return jsonError(403, "The tutor opens ten minutes after signup. You can use the lesson hints now.");

  // Dollar ceilings (per user and site-wide, per UTC day), checked before any quota is consumed.
  const budget = await checkBudget(db, user.id);
  if (!budget.ok) return jsonError(budget.scope === "user" ? 429 : 503, budget.message);

  // Only real model calls count against the quotas.
  const ip = clientIp(req.headers);
  const windowKey = (prefix: string, seconds: number) => `${prefix}:${Math.floor(Date.now() / (seconds * 1000))}`;
  const userHourKey = windowKey(`tutor:user:h:${user.id}`, HOUR);
  const userDayKey = windowKey(`tutor:user:d:${user.id}`, 24 * HOUR);
  const userHour = await rateLimitHit(`tutor:user:h:${user.id}`, LIMITS.userPerHour, HOUR);
  if (!userHour.ok)
    return jsonError(429, `You've used all ${LIMITS.userPerHour} tutor questions for this hour. They refill in about ${minutesUntil(userHour.resetAt)} minutes — meanwhile, the hints and "What probably went wrong" notes are free.`);
  const userDay = await rateLimitHit(`tutor:user:d:${user.id}`, LIMITS.userPerDay, 24 * HOUR);
  if (!userDay.ok) return jsonError(429, `You've reached today's limit of ${LIMITS.userPerDay} tutor questions. It resets in about ${Math.ceil(minutesUntil(userDay.resetAt) / 60)} hours.`);
  if (!(await rateLimit(`tutor:ip:${hmac("rate-ip", ip)}`, ipLimit(ip, LIMITS.ipPerHour), HOUR))) return jsonError(429, "Too many tutor questions from your network this hour. Please try again later.");
  const newcomer = age < 24 * 3_600_000;
  const newcomerKey = windowKey("tutor:new-accounts", HOUR);
  if (newcomer && !(await rateLimit("tutor:new-accounts", Math.max(1, Math.floor(LIMITS.globalPerHour / 4)), HOUR))) return jsonError(503, "The tutor is at capacity for new accounts. Please try again later.");
  if (!(await rateLimit("tutor:global", LIMITS.globalPerHour, HOUR))) {
    if (newcomer) await db.collection("rate_limits").updateOne({ key: newcomerKey }, { $inc: { count: -1 } });
    console.warn(`[tutor] global hourly limit (${LIMITS.globalPerHour}) reached`);
    return jsonError(503, "The tutor is at capacity right now. Please try again a little later.");
  }
  headers["X-Tutor-Remaining"] = String(Math.min(userHour.remaining, userDay.remaining));
  headers["X-Tutor-Limit"] = String(LIMITS.userPerHour);

  const abort = new AbortController();
  let cancelled = false;
  const disconnect = () => abort.abort();
  req.signal.addEventListener("abort", disconnect, { once: true });
  if (req.signal.aborted) disconnect();
  let usage: TutorUsage | null = null;
  let cacheable = false;
  const chunks = streamTutor(lessonCtx, buildMessages(thread?.messages ?? [], message, code, lastOutput), abort.signal, (u, ok) => {
    usage = u;
    cacheable = ok;
  });

  const readable = new ReadableStream<Uint8Array>({
    async start(controller) {
      let reply = "";
      try {
        for await (const text of chunks) {
          reply += text;
          if (reply.length > 32 * 1024) throw new TutorUnavailableError("The tutor response was too long. Try a more specific question.");
        }
        // Spend is recorded as soon as the API reports usage, even if the client left.
        if (usage) await recordSpend(db, user.id, usage as TutorUsage).catch((e) => console.error("[tutor] spend", e));
        if (abort.signal.aborted) return;
        if (exposesSolution(reply, found.exercise.solution, found.exercise.starter)) {
          reply = "Let's work through the idea one step at a time. Which part of the task is unclear?";
          cacheable = false;
        }
        await saveTurn(reply);
        if (!abort.signal.aborted) controller.enqueue(encoder.encode(reply));
        if (usage) await recordUsage({ ...(usage as TutorUsage), costMicroUsd: estimateCostMicro(usage as TutorUsage), responseCacheHit: false });
        if (cacheKey && cacheable && reply.trim()) {
          await db
            .collection("tutor_cache")
            .updateOne({ key: cacheKey }, { $setOnInsert: { key: cacheKey, reply, exerciseId: found.exercise.id, hits: 0, createdAt: new Date() } }, { upsert: true });
        }
      } catch (e) {
        let msg = "The tutor is unavailable right now. Please try again in a moment.";
        if (e instanceof TutorUnavailableError) msg = e.message;
        // Our failure, not theirs: give the question back.
        if (!reply) await db.collection("rate_limits").updateMany({ key: { $in: [userHourKey, userDayKey] } }, { $inc: { count: -1 } });
        else if (!abort.signal.aborted) console.error("[tutor]", e);
        if (!abort.signal.aborted) controller.enqueue(encoder.encode(`\n\n__TUTOR_ERROR__${msg}`));
      } finally {
        req.signal.removeEventListener("abort", disconnect);
        if (!cancelled) controller.close();
      }
    },
    cancel() {
      cancelled = true;
      abort.abort();
    },
  });

  return new Response(readable, { headers });
}

/** Clear the conversation for one lesson. */
export async function DELETE(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const user = await getCurrentUser();
  if (!user) return jsonError(401, "Not signed in.");
  if (!(await rateLimit(`tutor:delete:${user.id}`, 30, 60))) return jsonError(429, "Too many requests.");
  const exerciseId = new URL(req.url).searchParams.get("exerciseId");
  if (!exerciseId) return jsonError(400, "Missing exerciseId.");
  const db = await getDb();
  await db.collection("tutor_threads").deleteOne({ userId: new ObjectId(user.id), lessonId: exerciseId });
  return NextResponse.json({ ok: true });
}
