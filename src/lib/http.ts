import "server-only";
import { NextResponse } from "next/server";
import { SITE } from "./site";

/**
 * CSRF defence in depth for route handlers (server actions already check
 * Origin). Session cookies are SameSite=Strict, so cross-site requests carry
 * no session anyway; this also blocks cross-site anonymous POSTs.
 */
export function sameOrigin(req: Request): boolean {
  const origin = req.headers.get("origin");
  if (!origin) return req.headers.get("sec-fetch-site") === "same-origin";
  try {
    const o = new URL(origin);
    const host = req.headers.get("x-forwarded-host") ?? req.headers.get("host");
    return o.host === host || origin === SITE.url;
  } catch {
    return false;
  }
}

export const jsonError = (status: number, error: string) => NextResponse.json({ error }, { status });

export async function readJson<T>(req: Request, maxBytes = 128 * 1024): Promise<T | null> {
  const len = Number(req.headers.get("content-length") ?? 0);
  if (len > maxBytes) return null;
  const text = await req.text();
  if (text.length > maxBytes) return null;
  try {
    return JSON.parse(text) as T;
  } catch {
    return null;
  }
}
