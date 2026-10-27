"use client";

// Replaces the root layout if it fails, so it must render its own <html>.
export default function GlobalError({ reset }: { error: Error & { digest?: string }; reset: () => void }) {
  return (
    <html lang="en-GB">
      <body style={{ fontFamily: "system-ui, sans-serif", background: "#0e1013", color: "#e8eaee", display: "grid", placeItems: "center", minHeight: "100dvh", margin: 0 }}>
        <title>Something went wrong | Baremetal</title>
        <main style={{ textAlign: "center", padding: 24 }}>
          <h1 style={{ fontSize: 28 }}>Something went wrong</h1>
          <p style={{ color: "#9aa3b2" }}>Please try again in a moment.</p>
          <button type="button" onClick={reset} style={{ marginTop: 16, padding: "10px 20px", borderRadius: 8, border: 0, background: "#f5a524", color: "#1a1203", fontWeight: 600 }}>
            Try again
          </button>
        </main>
      </body>
    </html>
  );
}
