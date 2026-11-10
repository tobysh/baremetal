import { NextResponse } from "next/server";
import { adminFromRequest } from "@/lib/auth";
import { rateLimit } from "@/lib/rate-limit";
import { jsonError } from "@/lib/http";
import { getSiteStats } from "@/lib/stats";

// Authorization is enforced here on the server (role + 2FA), not in the UI.
export async function GET(req: Request) {
  const admin = await adminFromRequest();
  if (!admin) return jsonError(403, "Forbidden.");
  if (!(await rateLimit(`admin-stats:${admin.id}`, 30, 60))) return jsonError(429, "Too many requests.");
  const url = new URL(req.url);
  const days = Math.min(180, Math.max(1, Number(url.searchParams.get("days")) || 30));
  const stats = await getSiteStats(days);
  if (url.searchParams.get("format") === "csv") {
    const rows = ["day,pageviews,visitors", ...stats.daily.map((d) => `${d.day},${d.views},${d.visitors}`)];
    return new NextResponse(rows.join("\n") + "\n", { headers: { "Content-Type": "text/csv; charset=utf-8", "Content-Disposition": `attachment; filename="baremetal-daily-${days}d.csv"`, "Cache-Control": "no-store" } });
  }
  return NextResponse.json(stats, { headers: { "Cache-Control": "no-store" } });
}
