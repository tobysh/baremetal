import { ImageResponse } from "next/og";

export const OG_SIZE = { width: 1200, height: 630 };

/** Shared 1200×630 social card. Rendered at build time for every static route. */
export function ogImage({ eyebrow, title, subtitle }: { eyebrow: string; title: string; subtitle: string }) {
  return new ImageResponse(
    (
      <div style={{ width: "100%", height: "100%", display: "flex", flexDirection: "column", justifyContent: "space-between", background: "#0e1013", color: "#e8eaee", padding: 72 }}>
        <div style={{ display: "flex", alignItems: "center", gap: 18 }}>
          <svg width="56" height="56" viewBox="0 0 32 32">
            <rect width="32" height="32" rx="7" fill="#f5a524" />
            <path d="M9 10.5 14.5 16 9 21.5" fill="none" stroke="#1a1203" strokeWidth="2.8" strokeLinecap="round" strokeLinejoin="round" />
            <path d="M17 22h7" stroke="#1a1203" strokeWidth="2.8" strokeLinecap="round" />
          </svg>
          <span style={{ fontSize: 34, fontWeight: 700 }}>Baremetal</span>
        </div>
        <div style={{ display: "flex", flexDirection: "column", gap: 20 }}>
          <span style={{ fontSize: 28, color: "#f5a524" }}>{eyebrow}</span>
          <span style={{ fontSize: title.length > 40 ? 64 : 76, fontWeight: 700, lineHeight: 1.08, letterSpacing: -1.5 }}>{title}</span>
          <span style={{ fontSize: 30, color: "#9aa3b2", lineHeight: 1.35 }}>{subtitle.length > 140 ? `${subtitle.slice(0, 137)}…` : subtitle}</span>
        </div>
        <div style={{ display: "flex", fontSize: 24, color: "#9aa3b2" }}>Free · C, Rust, Go and assembly · In-browser code runner · AI tutor</div>
      </div>
    ),
    OG_SIZE,
  );
}
