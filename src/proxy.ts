import { readdir, readFile } from "node:fs/promises";
import path from "node:path";
import { NextResponse, type NextRequest } from "next/server";
import { contentSecurityPolicy, inlineScripts, legacyContentSecurityPolicy, scriptHash } from "@/lib/csp";
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
  const hashes = new Set<string>();
  const walk = async (dir: string): Promise<void> => {
    for (const entry of await readdir(dir, { withFileTypes: true })) {
      const file = path.join(dir, entry.name);
      if (entry.isDirectory()) await walk(file);
      else if (entry.name.endsWith(".html")) for (const body of inlineScripts(await readFile(file, "utf8"))) hashes.add(scriptHash(body));
    }
  };
  await walk(root).catch((error) => console.error("[csp] could not read the prerendered shells", error));
  if (!hashes.size) console.error("[csp] no prerendered shells found: falling back to script-src 'unsafe-inline'");
  return [...hashes].sort();
}

async function pagePolicy(): Promise<{ policy: string; nonce?: string }> {
  if (isDev) return { policy: contentSecurityPolicy({ dev: true }) };
  const hashes = await (shellHashes ??= inlineScriptHashes());
  if (!hashes.length) return { policy: legacyContentSecurityPolicy() };
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
  const { policy } = await pagePolicy();
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
