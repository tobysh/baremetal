"use client";

import { useId, useState } from "react";
import { highlight } from "./highlight";
import type { KindProps } from "./types";

/** Free recall: type a short answer. Matching happens on the server, so nothing about the accepted answers is sent down. */
export default function TypedPractice({ practice, language, locked, onAnswer }: KindProps<"typed">) {
  const [value, setValue] = useState("");
  const id = useId();
  return (
    <div>
      {practice.code && <pre tabIndex={0} className="overflow-x-auto rounded-xl border border-line-strong bg-[var(--code-bg)] p-3 font-mono text-[13px] text-[var(--code-text)]"><code>{highlight(practice.code, language)}</code></pre>}
      <label htmlFor={id} className="mt-3 block text-xs text-muted">Your answer</label>
      <input
        id={id}
        type="text"
        value={value}
        readOnly={locked}
        maxLength={200}
        autoComplete="off"
        autoCapitalize="off"
        autoCorrect="off"
        spellCheck={false}
        onChange={(e) => { setValue(e.target.value); onAnswer(e.target.value.trim() ? e.target.value : null); }}
        className="mt-1 min-h-11 w-full rounded-lg border border-line-strong bg-surface px-3 font-mono text-sm focus:border-accent focus:outline-none focus:ring-1 focus:ring-accent"
      />
    </div>
  );
}
