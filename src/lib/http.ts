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
    // Production compares against the configured site only: forwarded headers
    // are client-controlled unless every proxy overwrites them. Development
    // keeps the Host comparison so LAN and phone testing works.
    if (process.env.NODE_ENV === "production") return o.host === new URL(SITE.url).host;
    return o.host === req.headers.get("host") || origin === SITE.url;
  } catch {
    return false;
  }
}

export const jsonError = (status: number, error: string) => NextResponse.json({ error }, { status });

/**
 * Reads a JSON body without ever buffering more than `maxBytes`: the stream is
 * counted chunk by chunk and cancelled as soon as the cap is passed, so a
 * chunked request with no Content-Length can't make the process hold it all.
 */
export async function readJson<T>(req: Request, maxBytes = 128 * 1024): Promise<T | null> {
  if (Number(req.headers.get("content-length") ?? 0) > maxBytes || !req.body) return null;
  const reader = req.body.getReader();
  const chunks: Uint8Array[] = [];
  let size = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    size += value.byteLength;
    if (size > maxBytes) {
      await reader.cancel().catch(() => {});
      return null;
    }
    chunks.push(value);
  }
  try {
    return JSON.parse(Buffer.concat(chunks).toString("utf8")) as T;
  } catch {
    return null;
  }
}
