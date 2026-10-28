import type { Seals } from "@/lib/rewards-context";

// Three wax seals per lesson, drawn as simple shapes (never emoji). Earned seals are filled; unearned ones are
// a dashed outline, so the shape and the label carry the meaning, not just the colour.

type Kind = "passed" | "unaided" | "challenge";

const META: Record<Kind, { label: string; earned: string; locked: string; tone: string }> = {
  passed: { label: "Passed", earned: "Passed the tests", locked: "Not passed yet", tone: "text-accent" },
  unaided: { label: "Unaided", earned: "Passed without hints or the tutor", locked: "Unaided: pass without hints or the tutor, after at least one failed run (or on a hard lesson)", tone: "text-ok" },
  challenge: { label: "Challenge", earned: "Passed the challenge", locked: "Challenge not passed", tone: "text-text" },
};

function Shape({ kind, earned }: { kind: Kind; earned: boolean }) {
  const fill = earned ? "currentColor" : "none";
  const stroke = { stroke: "currentColor", strokeWidth: 1.6, strokeDasharray: earned ? undefined : "2.5 2.5" } as const;
  const mark = earned ? "var(--bg)" : "currentColor";
  if (kind === "passed")
    return (
      <svg viewBox="0 0 24 24" width="100%" height="100%" aria-hidden="true">
        <circle cx="12" cy="12" r="9.5" fill={fill} {...stroke} />
        <polyline points="7.5 12.5 10.7 15.5 16.5 9" fill="none" stroke={mark} strokeWidth="2.2" strokeLinecap="round" strokeLinejoin="round" />
      </svg>
    );
  if (kind === "unaided")
    return (
      <svg viewBox="0 0 24 24" width="100%" height="100%" aria-hidden="true">
        {/* An eight-pointed rosette, like the scalloped edge of a wax seal. */}
        <path d="M12 2.2l2.3 2.6 3.4-.5.7 3.4 3.1 1.6-1.2 3.2 1.2 3.2-3.1 1.6-.7 3.4-3.4-.5L12 21.8l-2.3-2.6-3.4.5-.7-3.4-3.1-1.6 1.2-3.2L2.5 8.3l3.1-1.6.7-3.4 3.4.5z" fill={fill} {...stroke} strokeLinejoin="round" />
        <circle cx="12" cy="12" r="3" fill="none" stroke={mark} strokeWidth="1.8" />
      </svg>
    );
  return (
    <svg viewBox="0 0 24 24" width="100%" height="100%" aria-hidden="true">
      <path d="M12 2.5l8 3v6.2c0 4.6-3.2 8.2-8 9.8-4.8-1.6-8-5.2-8-9.8V5.5z" fill={fill} {...stroke} strokeLinejoin="round" />
      <path d="M12 7.5l1.5 3 3.3.4-2.4 2.3.6 3.3-3-1.6-3 1.6.6-3.3-2.4-2.3 3.3-.4z" fill="none" stroke={mark} strokeWidth="1.4" strokeLinejoin="round" />
    </svg>
  );
}

export function Seal({ kind, earned, size = 20 }: { kind: Kind; earned: boolean; size?: number }) {
  const m = META[kind];
  return (
    <span role="img" aria-label={earned ? m.earned : m.locked} title={earned ? m.earned : m.locked} className={`inline-block shrink-0 ${m.tone} ${earned ? "" : "opacity-45"}`} style={{ width: size, height: size }}>
      <Shape kind={kind} earned={earned} />
    </span>
  );
}

/** The seals for one lesson. The Challenge seal only appears for lessons that have a challenge. */
export function SealRow({ seals, hasChallenge, size = 18 }: { seals: Seals; hasChallenge: boolean; size?: number }) {
  return (
    <span className="inline-flex items-center gap-1">
      <Seal kind="passed" earned={seals.passed} size={size} />
      <Seal kind="unaided" earned={seals.unaided} size={size} />
      {hasChallenge && <Seal kind="challenge" earned={seals.challenge} size={size} />}
    </span>
  );
}

export function SealLegend() {
  return (
    <ul className="flex flex-wrap gap-x-5 gap-y-1 text-xs text-muted">
      {(Object.keys(META) as Kind[]).map((k) => (
        <li key={k} className="inline-flex items-center gap-1.5">
          <Seal kind={k} earned size={14} /> {META[k].label}
        </li>
      ))}
    </ul>
  );
}
