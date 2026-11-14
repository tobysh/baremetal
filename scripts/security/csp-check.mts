// CSP regression check against a production server (`next start`; `next dev` uses a relaxed policy).
//   npx tsx scripts/security/csp-check.mts http://127.0.0.1:3100 [--no-browser]
// 1. Headers and HTML: every page has a per-request nonce and no 'unsafe-inline' for scripts, and every
//    executable inline script in the served HTML either carries that nonce or has its hash in the policy.
// 2. Browser (Playwright Chromium): loading the pages and navigating client-side raises zero CSP
//    violations, React hydrates, the theme bootstrap runs, the editor mounts, the service worker
//    registers and the analytics beacon is sent.
import assert from "node:assert/strict";
import { inlineScripts, scriptHash } from "../../src/lib/csp";

const base = (process.argv.slice(2).find((a) => !a.startsWith("--")) ?? "http://127.0.0.1:3100").replace(/\/$/, "");
const browser = !process.argv.includes("--no-browser");
const PAGES = ["/", "/learn", "/learn/first-steps/hello-world", "/login"];
// Header and HTML checks also cover other shells and a dynamic not-found render.
const HEADER_PAGES: [string, number][] = [...PAGES.map((p): [string, number] => [p, 200]), ["/learn/first-steps", 200], ["/leaderboard", 200], ["/signup", 200], ["/offline", 200], ["/no-such-page", 404]];

const directives = (csp: string) => new Map(csp.split(";").map((d) => d.trim().split(/\s+/)).filter((p) => p[0]).map(([name, ...v]) => [name, v]));

async function headerChecks() {
  const nonces = new Set<string>();
  for (const [path, status] of HEADER_PAGES) {
    for (let i = 0; i < 2; i++) {
      const res = await fetch(base + path, { redirect: "manual" });
      assert.equal(res.status, status, `${path} status`);
      const raw = res.headers.get("content-security-policy") ?? "";
      assert.ok(raw && !raw.includes(", "), `${path}: exactly one CSP header (got ${JSON.stringify(raw.slice(0, 80))})`);
      const csp = directives(raw);
      const script = csp.get("script-src") ?? [];
      const nonce = script.find((s) => s.startsWith("'nonce-"))?.slice(7, -1);
      assert.ok(nonce && nonce.length >= 22, `${path}: script-src has a nonce`);
      assert.ok(!nonces.has(nonce), `${path}: nonce is fresh per request`);
      nonces.add(nonce);
      assert.ok(!script.includes("'unsafe-inline'") && !script.includes("'unsafe-eval'"), `${path}: no unsafe-inline/unsafe-eval in script-src`);
      assert.ok(script.every((s) => s === "'self'" || s.startsWith("'nonce-") || s.startsWith("'sha256-")), `${path}: script-src is only 'self', nonce and hashes`);
      for (const [name, value] of [["default-src", "'self'"], ["object-src", "'none'"], ["base-uri", "'self'"], ["form-action", "'self'"], ["frame-ancestors", "'none'"], ["connect-src", "'self'"]]) {
        assert.deepEqual(csp.get(name), [value], `${path}: ${name} ${value}`);
      }
      const html = await res.text();
      // Every executable inline script must be allowed by this policy, or the page would not hydrate.
      for (const m of html.matchAll(/<script\b([^>]*)>([\s\S]*?)<\/script>/gi)) {
        if (!inlineScripts(m[0]).length) continue;
        const ok = new RegExp(`\\snonce="${nonce.replace(/[+/=]/g, "\\$&")}"`).test(m[1]) || script.includes(scriptHash(m[2]));
        assert.ok(ok, `${path}: inline script allowed by nonce or hash: ${m[2].slice(0, 80)}`);
      }
      // A nonce in the HTML other than this response's would mean a cached per-request render.
      for (const m of html.matchAll(/\snonce="([^"]*)"/g)) assert.equal(m[1], nonce, `${path}: HTML nonces match the header`);
    }
  }
  // Responses the proxy does not see get the static fallback: still no inline script at all.
  for (const path of ["/api/tutor/quota", "/robots.txt", "/sw.js"]) {
    const script = directives((await fetch(base + path)).headers.get("content-security-policy") ?? "").get("script-src");
    assert.deepEqual(script, ["'self'"], `${path}: fallback script-src 'self'`);
  }
  // The signed-out redirect for protected pages still happens in the proxy.
  const redirect = await fetch(base + "/dashboard", { redirect: "manual" });
  assert.equal(redirect.status, 307, "/dashboard redirects when signed out");
  assert.match(redirect.headers.get("location") ?? "", /\/login\?next=%2Fdashboard$/);
  console.log("CSP header and inline-script assertions passed.");
}

async function browserChecks() {
  const { chromium } = await import("playwright");
  const b = await chromium.launch({
    ...(process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE ? { executablePath: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE } : {}),
    args: process.env.BM_REGRESSION_HTTPS === "1" && process.env.BM_REGRESSION_TLS_SPKI
      ? [`--ignore-certificate-errors-spki-list=${process.env.BM_REGRESSION_TLS_SPKI}`] : [],
  });
  try {
    const context = await b.newContext({ baseURL: base });
    const url = new URL(base);
    await context.addCookies([{ name: "bm_consent", value: encodeURIComponent(JSON.stringify({ v: 1, analytics: true, at: new Date().toISOString() })), domain: url.hostname, path: "/" }]);
    await context.addInitScript(() => {
      const w = window as unknown as { __csp: string[] };
      w.__csp = [];
      document.addEventListener("securitypolicyviolation", (e) => w.__csp.push(`${e.violatedDirective} ${e.blockedURI} ${e.sample}`));
      try { localStorage.setItem("bm:theme", "light"); } catch {}
    });
    const page = await context.newPage();
    const problems: string[] = [];
    page.on("console", (m) => { if (/Content.Security.Policy|Refused to (execute|load|apply)/i.test(m.text())) problems.push(m.text()); });
    page.on("pageerror", (e) => problems.push(`pageerror: ${e.message}`));
    const violations = () => page.evaluate(() => (window as unknown as { __csp: string[] }).__csp);
    const hydrated = () => page.evaluate(() => Object.keys(document.getElementById("main") ?? {}).some((k) => k.startsWith("__react")));

    for (const path of PAGES) {
      const beacon = page.waitForRequest((r) => r.url().endsWith("/api/analytics"), { timeout: 15_000 }).catch(() => null);
      await page.goto(path, { waitUntil: "load" });
      await page.waitForFunction(() => Object.keys(document.getElementById("main") ?? {}).some((k) => k.startsWith("__react")), null, { timeout: 15_000 }).catch(() => {});
      assert.ok(await hydrated(), `${path}: React hydrated`);
      assert.equal(await page.getAttribute("html", "data-theme"), "light", `${path}: theme bootstrap ran`);
      if (path.startsWith("/learn/first-steps/")) await page.locator(".cm-editor").first().waitFor({ timeout: 20_000 });
      assert.ok(await beacon, `${path}: analytics beacon sent`);
      await page.waitForTimeout(300);
      assert.deepEqual(await violations(), [], `${path}: no CSP violations`);
    }
    const sw = await page.evaluate(() => Promise.race([navigator.serviceWorker.ready.then((r) => r.scope), new Promise((r) => setTimeout(() => r(null), 10_000))]));
    assert.equal(sw, `${base}/`, "service worker registered");
    // Client-side navigation (RSC fetches, no full load) stays clean too.
    await page.goto("/", { waitUntil: "load" });
    await page.waitForFunction(() => Object.keys(document.getElementById("main") ?? {}).some((k) => k.startsWith("__react")));
    await page.locator('a[href="/learn"]').first().click();
    await page.waitForURL("**/learn");
    await page.waitForTimeout(500);
    assert.deepEqual(await violations(), [], "client navigation: no CSP violations");
    assert.deepEqual(problems, [], "no CSP console messages or page errors");
    console.log(`Browser CSP checks passed (${PAGES.join(", ")}, client navigation, service worker, analytics).`);
  } finally {
    await b.close();
  }
}

await headerChecks();
if (browser) await browserChecks();
