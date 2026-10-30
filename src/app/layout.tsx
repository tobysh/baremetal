import { NavProgress } from "@/components/nav-progress";
import type { Metadata, Viewport } from "next";
import { Archivo, IBM_Plex_Mono, IBM_Plex_Sans } from "next/font/google";
import { Analytics } from "@/components/analytics";
import { CookieBanner } from "@/components/consent";
import { InlineScript } from "@/components/inline-script";
import { SiteFooter } from "@/components/site-footer";
import { SiteHeader } from "@/components/site-header";
import { themeInitScript } from "@/components/theme-toggle";
import { SITE } from "@/lib/site";
import { Suspense } from "react";
import "./globals.css";

const plexSans = IBM_Plex_Sans({ variable: "--font-plex-sans", subsets: ["latin"], weight: ["400", "500", "600", "700"], display: "swap" });
// Display face for headings on the dashboard; the width axis gives the wide cut (font-stretch).
const archivo = Archivo({ variable: "--font-archivo", subsets: ["latin"], axes: ["wdth"], display: "swap" });
const plexMono = IBM_Plex_Mono({ variable: "--font-plex-mono", subsets: ["latin"], weight: ["400", "500"], display: "swap" });

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
          <Analytics />
        </Suspense>
      </body>
    </html>
  );
}
