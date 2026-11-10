import type { Db, Document } from "mongodb";

const MINUTE = 60_000;

/** Aggregate operational evidence only. No account identifiers, code or scope keys leave the database. */
export async function accountingHealth(db: Db, now = new Date()) {
  const overdueRecovery = new Date(+now - 10 * MINUTE);
  const abandonedCall = new Date(+now - 60 * MINUTE);
  const count = (collection: string, filter: Document) => db.collection(collection).countDocuments(filter, { maxTimeMS: 30_000 });
  const [pendingReviewEvents, staleReviewEvents, unresolvedReservations, staleReservations, pendingXp, staleXp, orphanedPersonalBalances, largeSpendDocuments] = await Promise.all([
    count("reviews", { pendingEvent: { $exists: true } }),
    count("reviews", { pendingEvent: { $exists: true }, $or: [{ "pendingEvent.at": { $lte: overdueRecovery } }, { "pendingEvent.at": { $not: { $type: "date" } } }] }),
    count("tutor_reservations", { state: { $ne: "settled" } }),
    count("tutor_reservations", { $or: [
      { state: "settling", createdAt: { $lte: overdueRecovery } },
      { state: { $in: ["reserving", "active"] }, createdAt: { $lte: abandonedCall } },
      { state: { $ne: "settled" }, createdAt: { $not: { $type: "date" } } },
      { state: { $nin: ["reserving", "active", "settling", "settled"] } },
    ] }),
    count("progress", { xpPending: true }),
    count("progress", { xpPending: true, $or: [{ passedAt: { $lte: overdueRecovery } }, { passedAt: { $not: { $type: "date" } } }] }),
    db.collection("tutor_spend").aggregate<{ count: number }>([
      { $match: { key: /^u:/ } },
      { $set: { ownerId: { $convert: { input: { $arrayElemAt: [{ $split: ["$key", ":"] }, 1] }, to: "objectId", onError: null, onNull: null } } } },
      { $lookup: { from: "users", localField: "ownerId", foreignField: "_id", as: "owner" } },
      { $match: { owner: { $size: 0 } } },
      { $count: "count" },
    ], { maxTimeMS: 30_000, allowDiskUse: false }).toArray(),
    // Marker maps are bounded by MongoDB's 16 MiB document limit; warn with room to investigate.
    db.collection("tutor_spend").aggregate<{ count: number }>([
      { $match: { $expr: { $gte: [{ $bsonSize: "$$ROOT" }, 8 * 1024 * 1024] } } },
      { $count: "count" },
    ], { maxTimeMS: 30_000, allowDiskUse: false }).toArray(),
  ]);
  const counts = {
    pendingReviewEvents, staleReviewEvents, unresolvedReservations, staleReservations, pendingXp, staleXp,
    orphanedPersonalBalances: orphanedPersonalBalances[0]?.count ?? 0,
    largeSpendDocuments: largeSpendDocuments[0]?.count ?? 0,
  };
  return {
    checkedAt: now.toISOString(),
    needsAttention: !!(counts.staleReviewEvents || counts.staleReservations || counts.staleXp || counts.orphanedPersonalBalances || counts.largeSpendDocuments),
    counts,
  };
}
