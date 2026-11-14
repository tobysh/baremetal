import path from "node:path";
import { NextResponse, type NextRequest } from "next/server";
import { contentSecurityPolicy } from "@/lib/csp";
import { loadShellScriptHashes } from "@/lib/csp-shells";
import { SESSION_COOKIE } from "@/lib/session-cookie";

const isDev = process.env.NODE_ENV === "development";
const PROTECTED = /^\/(dashboard|settings|admin)(\/|$)/;

/**
 * Hashes of every executable inline script in the prerendered shells (.next/server/app/**.html).
 * Those scripts were rendered at build time without a request, so they cannot carry a nonce; in
 * practice they are the theme bootstrap and React's reveal runtime. Read once per server process.
 */
let shellHashes: Promise<string[]> | undefined;
async function inlineScriptHashes(): Promise<string[]> {
  const root = path.join(process.cwd(), ".next", "server", "app");
  return loadShellScriptHashes(root).catch((error) => {
    console.error("[csp] prerendered shell validation failed; page serving disabled. Restore the complete .next build and restart the server.", error);
    throw error;
  });
}

async function pagePolicy(): Promise<{ policy: string; nonce?: string }> {
  if (isDev) return { policy: contentSecurityPolicy({ dev: true }) };
  const hashes = await (shellHashes ??= inlineScriptHashes());
  const nonce = Buffer.from(crypto.getRandomValues(new Uint8Array(18))).toString("base64");
  return { policy: contentSecurityPolicy({ nonce, scriptHashes: hashes }), nonce };
}

export async function proxy(req: NextRequest) {
  // Optimistic check only: sends signed-out visitors straight to the login page
  // instead of rendering a protected page shell first. The real checks (valid
  // session, admin role, 2FA) run on the server in requireUser/requireAdmin,
  // and in every route handler.
  if (PROTECTED.test(req.nextUrl.pathname) && !req.cookies.has(SESSION_COOKIE)) {
    const url = new URL("/login", req.url);
    url.searchParams.set("next", req.nextUrl.pathname);
    return NextResponse.redirect(url);
  }
  // Per-request CSP. Next reads the nonce from the request's Content-Security-Policy header and
  // puts it on the scripts it streams for this request.
  let policy: string;
  try {
    ({ policy } = await pagePolicy());
  } catch {
    return new NextResponse("Temporarily unavailable", {
      status: 503,
      headers: {
        "Content-Type": "text/plain; charset=utf-8",
        "Cache-Control": "no-store",
        "Content-Security-Policy": contentSecurityPolicy(),
      },
    });
  }
  const headers = new Headers(req.headers);
  headers.set("Content-Security-Policy", policy);
  const res = NextResponse.next({ request: { headers } });
  res.headers.set("Content-Security-Policy", policy);
  return res;
}

export const config = {
  // Every page. API routes, build assets and the metadata/static files keep the fallback policy from next.config.ts.
  matcher: ["/((?!api/|_next/static/|_next/image|favicon\\.ico|sw\\.js|robots\\.txt|sitemap\\.xml|manifest\\.webmanifest|llms\\.txt|icon|apple-icon|opengraph-image).*)"],
};
