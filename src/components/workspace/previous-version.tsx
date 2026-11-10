"use client";

import { useState, useSyncExternalStore } from "react";

const noop = () => () => {};

/** The unsuffixed draft key a lesson used before its first revision (see storageKey in workspace.tsx). */
function readLegacyDraft(key: string): string | null {
  try {
    return localStorage.getItem(key);
  } catch {
    return null;
  }
}

/**
 * Shown when a lesson has been rewritten (its `revision` went up). The editor opens on the new
 * starter, and the learner's code from before is kept here, collapsed, so nothing is lost.
 */
export function PreviousVersion({ exerciseId, serverCode, signedIn, draftOwner }: { exerciseId: string; serverCode: string | null; signedIn: boolean; draftOwner?: string }) {
  const legacyKey = signedIn ? `bm:draft:${draftOwner}:${exerciseId}` : `bm:code:${exerciseId}`;
  const local = useSyncExternalStore(noop, () => readLegacyDraft(legacyKey), () => null);
  const [copied, setCopied] = useState(false);
  const code = (local && local.trim() ? local : serverCode)?.trimEnd();
  if (!code) return null;
  return (
    <details className="mb-2 panel p-3 text-sm">
      <summary className="min-h-9 cursor-pointer text-muted">This lesson was rewritten. Show your previous version.</summary>
      <p className="mt-2 text-muted">The task is different now, so the editor starts fresh. Your earlier code is below. It is not used for testing.</p>
      <pre className="mt-2 max-h-60 overflow-auto rounded-lg border border-border bg-bg p-3 font-mono text-xs leading-5 whitespace-pre-wrap">{code}</pre>
      <button
        type="button"
        className="mt-2 min-h-9 rounded-md border border-border px-3 text-xs hover:bg-surface-2"
        onClick={() => {
          navigator.clipboard?.writeText(code).then(() => setCopied(true), () => {});
        }}
      >
        {copied ? "Copied" : "Copy previous code"}
      </button>
    </details>
  );
}
