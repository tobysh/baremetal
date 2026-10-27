import Link from "next/link";
import { Suspense } from "react";
import { SITE } from "@/lib/site";
import { CookieSettingsButton } from "./consent";
import { Container } from "./ui";
import { LogoMark } from "./logo";
import { FooterFrame } from "./footer-frame";

export function SiteFooter() {
  return (
    <Suspense fallback={null}>
    <FooterFrame legal={<Container className="py-5 text-xs text-muted">© 2026 {SITE.legal.entity}</Container>}>
      <Container className="grid gap-8 border-b border-border py-10 text-sm sm:grid-cols-[1.5fr_1fr_1fr]">
        <div>
          <div className="flex items-center gap-2 font-semibold">
            <LogoMark className="h-6 w-6" /> {SITE.name}
          </div>
          <p className="mt-3 max-w-xs text-muted">Learn how computers really work by writing C, Rust, Go and assembly, one small step at a time.</p>
        </div>
        <nav aria-label="Learn">
          <h2 className="font-medium">Learn</h2>
          <ul className="mt-3 space-y-2 text-muted">
            <li><Link className="hover:text-text" href="/learn">All courses</Link></li>
            <li><Link className="hover:text-text" href="/learn/first-steps">Programming from Zero</Link></li>
            <li><Link className="hover:text-text" href="/learn/rust-http">Build an HTTP server</Link></li>
          </ul>
        </nav>
        <nav aria-label="Legal">
          <h2 className="font-medium">Legal</h2>
          <ul className="mt-3 space-y-2 text-muted">
            <li><Link className="hover:text-text" href="/terms">Terms of Service</Link></li>
            <li><Link className="hover:text-text" href="/privacy">Privacy Notice</Link></li>
            <li><Link className="hover:text-text" href="/cookies">Cookie Policy</Link></li>
            <li><CookieSettingsButton /></li>
          </ul>
        </nav>
      </Container>
    </FooterFrame>
    </Suspense>
  );
}
