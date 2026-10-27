import "server-only";
import { io } from "next/cache";
import { getDb } from "./db";

export type SiteStats = {
  days: number;
  pageviews: number;
  visitors: number;
  topPages: { path: string; views: number }[];
  topReferrers: { ref: string; visits: number }[];
  devices: { device: string; count: number }[];
  daily: { day: string; views: number; visitors: number }[];
  funnel: { signups: number; testPasses: number };
  security: { failedLogins: number; lockouts: number; rateLimited: number };
  tutor: { requests: number; responseCacheHits: number; inputTokens: number; outputTokens: number; cacheReadTokens: number; cacheWriteTokens: number };
};

export async function getSiteStats(days = 30): Promise<SiteStats> {
  await io();
  const db = await getDb();
  const since = new Date(Date.now() - days * 86_400_000);
  const events = db.collection("events");
  const pv = { type: "pageview", ts: { $gte: since } };
  const [totals, topPages, topReferrers, devices, daily, signups, testPasses, failedLogins, lockouts, rateLimited, tutor] = await Promise.all([
    events.aggregate([{ $match: pv }, { $group: { _id: null, views: { $sum: 1 }, visitors: { $addToSet: "$visitor" } } }]).toArray(),
    events.aggregate([{ $match: pv }, { $group: { _id: "$path", views: { $sum: 1 } } }, { $sort: { views: -1 } }, { $limit: 10 }]).toArray(),
    events
      .aggregate([{ $match: { ...pv, ref: { $ne: null } } }, { $group: { _id: "$ref", visits: { $sum: 1 } } }, { $sort: { visits: -1 } }, { $limit: 10 }])
      .toArray(),
    events.aggregate([{ $match: pv }, { $group: { _id: "$device", count: { $sum: 1 } } }]).toArray(),
    events
      .aggregate([
        { $match: pv },
        { $group: { _id: { $dateToString: { format: "%Y-%m-%d", date: "$ts" } }, views: { $sum: 1 }, visitors: { $addToSet: "$visitor" } } },
        { $sort: { _id: 1 } },
      ])
      .toArray(),
    db.collection("auth_events").countDocuments({ type: "signup", ts: { $gte: since } }),
    db.collection("progress").countDocuments({ status: "passed", passedAt: { $gte: since } }),
    db.collection("auth_events").countDocuments({ type: "login_failed", ts: { $gte: since } }),
    db.collection("auth_events").countDocuments({ type: "login_locked", ts: { $gte: since } }),
    db.collection("auth_events").countDocuments({ type: "rate_limited", ts: { $gte: since } }),
    db
      .collection("tutor_usage")
      .aggregate([
        { $match: { ts: { $gte: since } } },
        {
          $group: {
            _id: null,
            requests: { $sum: 1 },
            responseCacheHits: { $sum: { $cond: ["$responseCacheHit", 1, 0] } },
            inputTokens: { $sum: { $ifNull: ["$inputTokens", 0] } },
            outputTokens: { $sum: { $ifNull: ["$outputTokens", 0] } },
            cacheReadTokens: { $sum: { $ifNull: ["$cacheReadTokens", 0] } },
            cacheWriteTokens: { $sum: { $ifNull: ["$cacheWriteTokens", 0] } },
          },
        },
      ])
      .toArray(),
  ]);
  return {
    days,
    pageviews: totals[0]?.views ?? 0,
    visitors: totals[0]?.visitors.length ?? 0,
    topPages: topPages.map((p) => ({ path: p._id, views: p.views })),
    topReferrers: topReferrers.map((r) => ({ ref: r._id, visits: r.visits })),
    devices: devices.map((d) => ({ device: d._id, count: d.count })),
    daily: daily.map((d) => ({ day: d._id, views: d.views, visitors: d.visitors.length })),
    funnel: { signups, testPasses },
    security: { failedLogins, lockouts, rateLimited },
    tutor: {
      requests: tutor[0]?.requests ?? 0,
      responseCacheHits: tutor[0]?.responseCacheHits ?? 0,
      inputTokens: tutor[0]?.inputTokens ?? 0,
      outputTokens: tutor[0]?.outputTokens ?? 0,
      cacheReadTokens: tutor[0]?.cacheReadTokens ?? 0,
      cacheWriteTokens: tutor[0]?.cacheWriteTokens ?? 0,
    },
  };
}
