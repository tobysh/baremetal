"use client";

/**
 * An inline script that runs synchronously while the HTML is parsed (before
 * first paint). It's executable only in the server-rendered HTML; when React
 * renders it in the browser it's inert text/plain, which avoids React's
 * "script tag" warning. Pattern from Next's "preventing flash before
 * hydration" guide.
 */
export function InlineScript({ html }: { html: string }) {
  return <script type={typeof window === "undefined" ? "text/javascript" : "text/plain"} suppressHydrationWarning dangerouslySetInnerHTML={{ __html: html }} />;
}
