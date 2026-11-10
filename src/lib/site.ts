// The operator is only shown once LEGAL_ENTITY_NAME holds a real value. The
// placeholder used during development is ignored so the footer falls back to the brand.
const configuredOperator = process.env.LEGAL_ENTITY_NAME?.trim() ?? "";
const operator = configuredOperator && !/to be confirmed/i.test(configuredOperator) ? configuredOperator : null;

export const SITE = {
  name: "Baremetal",
  tagline: "Learn low-level programming from zero, with an AI tutor that never just hands you the answer.",
  description:
    "Learn C, Rust, Go and x86-64 assembly from absolute zero. Write real code in your browser, get instant test feedback, and work through problems with an AI tutor — from reversing strings to building an HTTP server.",
  url: (process.env.NEXT_PUBLIC_SITE_URL ?? "http://localhost:3000").replace(/\/$/, ""),
  // Legal entity details used across the Terms, Privacy and Cookie pages.
  // Replace these before going live.
  legal: {
    entity: process.env.LEGAL_ENTITY_NAME ?? "Baremetal (operator name to be confirmed)",
    /** The configured operator, or null while the name is unset or still a placeholder. */
    operator,
    /** Name shown after the copyright sign in the footers. */
    copyrightHolder: operator ?? "Baremetal",
    address: process.env.LEGAL_ADDRESS ?? "Registered address to be confirmed",
    contactEmail: process.env.LEGAL_CONTACT_EMAIL ?? "privacy@example.com",
    jurisdiction: process.env.LEGAL_JURISDICTION ?? "England and Wales",
    lastUpdated: "10 November 2026",
  },
} as const;

export function absoluteUrl(path = "/") {
  return `${SITE.url}${path.startsWith("/") ? path : `/${path}`}`;
}
