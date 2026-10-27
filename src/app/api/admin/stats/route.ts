import { NextResponse } from "next/server";
import { adminFromRequest } from "@/lib/auth";
import { jsonError } from "@/lib/http";
import { getSiteStats } from "@/lib/stats";

// Authorization is enforced here on the server (role + 2FA), not in the UI.
export async function GET(req: Request) {
  const admin = await adminFromRequest();
  if (!admin) return jsonError(403, "Forbidden.");
  const days = Math.min(180, Math.max(1, Number(new URL(req.url).searchParams.get("days")) || 30));
  return NextResponse.json(await getSiteStats(days), { headers: { "Cache-Control": "no-store" } });
}
