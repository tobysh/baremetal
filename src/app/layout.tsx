import { NavProgress } from "@/components/nav-progress";
import type { Metadata, Viewport } from "next";
import localFont from "next/font/local";
import { Analytics } from "@/components/analytics";
import { CookieBanner } from "@/components/consent";
import { InlineScript } from "@/components/inline-script";
import { ServiceWorker } from "@/components/service-worker";
import { SiteFooter } from "@/components/site-footer";
import { SiteHeader } from "@/components/site-header";
import { themeInitScript } from "@/components/theme-toggle";
import { SITE } from "@/lib/site";
import { Suspense } from "react";
import "./globals.css";

// Fonts are committed under ./fonts (SIL OFL, see fonts/LICENSE.md) so builds need no network.
const plexSans = localFont({
  variable: "--font-plex-sans",
  display: "swap",
  src: [
    { path: "./fonts/ibm-plex-sans-latin-400-normal.woff2", weight: "400", style: "normal" },
    { path: "./fonts/ibm-plex-sans-latin-500-normal.woff2", weight: "500", style: "normal" },
    { path: "./fonts/ibm-plex-sans-latin-600-normal.woff2", weight: "600", style: "normal" },
    { path: "./fonts/ibm-plex-sans-latin-700-normal.woff2", weight: "700", style: "normal" },
  ],
});
// Display face for headings on the dashboard; the width axis gives the wide cut (font-stretch).
const archivo = localFont({
  variable: "--font-archivo",
  display: "swap",
  src: [{ path: "./fonts/archivo-latin-wdth-normal.woff2", weight: "100 900", style: "normal" }],
  declarations: [{ prop: "font-stretch", value: "62% 125%" }],
});
const plexMono = localFont({
  variable: "--font-plex-mono",
  display: "swap",
  src: [
    { path: "./fonts/ibm-plex-mono-latin-400-normal.woff2", weight: "400", style: "normal" },
    { path: "./fonts/ibm-plex-mono-latin-500-normal.woff2", weight: "500", style: "normal" },
  ],
});

export const metadata: Metadata = {
  metadataBase: new URL(SITE.url),
  title: { default: `${SITE.name} — Learn low-level programming from zero`, template: `%s | ${SITE.name}` },
  description: SITE.description,
  applicationName: SITE.name,
  openGraph: { type: "website", siteName: SITE.name, locale: "en_GB" },
  twitter: { card: "summary_large_image" },
  formatDetection: { telephone: false, email: false, address: false },
};

export const viewport: Viewport = {
  viewportFit: "cover",
  themeColor: [
    { media: "(prefers-color-scheme: dark)", color: "#0e1013" },
    { media: "(prefers-color-scheme: light)", color: "#fbfaf7" },
  ],
};

export default function RootLayout({ children }: LayoutProps<"/">) {
  return (
    // suppressHydrationWarning: the pre-paint script may set data-theme before React hydrates.
    <html lang="en-GB" className={`${plexSans.variable} ${plexMono.variable} ${archivo.variable} antialiased`} suppressHydrationWarning>
      <head>
        {/* Sets the saved theme before first paint, so there's no flash of the wrong one. */}
        <InlineScript html={themeInitScript} />
      </head>
      <body className="flex min-h-dvh flex-col">
        <a href="#main" className="sr-only z-50 rounded-md bg-accent px-3 py-2 text-accent-ink focus:not-sr-only focus:fixed focus:left-3 focus:top-3">
          Skip to content
        </a>
        <SiteHeader />
        <main id="main" tabIndex={-1} className="flex-1 focus:outline-none">
          {children}
        </main>
        <SiteFooter />
        <CookieBanner />
        {/* usePathname() needs a boundary on routes whose URL isn't known at build time (e.g. /u/[username]). */}
        <Suspense fallback={null}>
          <NavProgress />
          <ServiceWorker />
          <Analytics />
        </Suspense>
      </body>
    </html>
  );
}
