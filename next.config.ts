import type { NextConfig } from "next";
import { contentSecurityPolicy } from "./src/lib/csp";

const isDev = process.env.NODE_ENV === "development";

// No third-party origins at all: fonts are self-hosted by next/font, analytics
// is first-party, and the tutor is called from the server. Pages get a per-request
// policy with a nonce and the shell-script hashes from src/proxy.ts; this static one
// (no inline scripts at all) covers what the proxy skips: API routes, build assets
// and static files.
const csp = contentSecurityPolicy({ dev: isDev });

const securityHeaders = [
  { key: "Content-Security-Policy", value: csp },
  { key: "X-Content-Type-Options", value: "nosniff" },
  { key: "X-Frame-Options", value: "DENY" },
  { key: "Referrer-Policy", value: "strict-origin-when-cross-origin" },
  { key: "Permissions-Policy", value: "camera=(), microphone=(), geolocation=(), payment=(), usb=(), browsing-topics=()" },
  { key: "Cross-Origin-Opener-Policy", value: "same-origin" },
  ...(isDev ? [] : [{ key: "Strict-Transport-Security", value: "max-age=63072000; includeSubDomains; preload" }]),
];

const nextConfig: NextConfig = {
  cacheComponents: true,
  partialPrefetching: true,
  poweredByHeader: false,
  // Dev only: lets a phone on the LAN load /_next/* dev resources (HMR); otherwise the page never hydrates.
  allowedDevOrigins: (process.env.DEV_ORIGINS ?? "192.168.5.150").split(",").map((o) => o.trim()).filter(Boolean),
  // Don't publish source maps for browser bundles in production.
  productionBrowserSourceMaps: false,
  // Lesson content, legal text and the password list are read from disk at runtime.
  outputFileTracingIncludes: { "/**": ["./content/**/*", "./data/**/*"] },
  async headers() {
    return [
      { source: "/:path*", headers: securityHeaders },
      { source: "/api/:path*", headers: [{ key: "Cache-Control", value: "no-store" }] },
      // The worker script itself must always be revalidated so updates reach installed copies.
      { source: "/sw.js", headers: [{ key: "Cache-Control", value: "no-cache" }, { key: "Service-Worker-Allowed", value: "/" }] },
    ];
  },
};

export default nextConfig;
