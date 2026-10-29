import "server-only";
import { getDb } from "./db";
import { POPULAR_HITS } from "./tutor-cache";

export type PopularAnswer = { exerciseId: string; kind: string; question: string | null; reply: string; hits: number; lastHitAt: Date | null };

/** Most-served cached tutor answers, for admins. No user data: entries are not linked to accounts. */
export async function getPopularTutorAnswers(limit = 50): Promise<PopularAnswer[]> {
  const db = await getDb();
  const docs = await db.collection("tutor_cache").find({ hits: { $gte: POPULAR_HITS } }).sort({ hits: -1 }).limit(limit).toArray();
  return docs.map((d) => ({ exerciseId: String(d.exerciseId ?? ""), kind: String(d.kind ?? "exact"), question: typeof d.question === "string" ? d.question : null, reply: String(d.reply), hits: Number(d.hits), lastHitAt: d.lastHitAt ?? null }));
}
