import { DIFFICULTIES, levelFor, type Difficulty } from "@/lib/gamification";
import { ProgressBar } from "./ui";

// Green, amber, red, dark red (see CLAUDE.md). Impossible is solid so it reads
// as a step beyond Hard rather than another shade of it.
const STYLES: Record<Difficulty, string> = {
  easy: "bg-easy/12 text-easy border-easy/35",
  medium: "bg-medium/12 text-medium border-medium/35",
  hard: "bg-hard/12 text-hard border-hard/35",
  impossible: "bg-impossible text-impossible-ink border-impossible",
};

export function DifficultyBadge({ difficulty, showXp = true }: { difficulty: Difficulty; showXp?: boolean }) {
  const d = DIFFICULTIES[difficulty];
  return (
    <span className={`inline-flex items-center gap-1 rounded-full border px-2.5 py-0.5 text-xs font-semibold ${STYLES[difficulty]}`}>
      {d.label}
      {showXp && <span className="font-normal opacity-90">· {d.xp} XP</span>}
    </span>
  );
}

export function LevelChip({ xp, className = "" }: { xp: number; className?: string }) {
  const l = levelFor(xp);
  return (
    <span className={`inline-flex items-center gap-1.5 rounded-full border border-accent/40 bg-accent-soft px-2.5 py-0.5 text-xs font-semibold text-text ${className}`}>
      <span className="text-accent">Lv {l.level}</span>
      <span className="font-normal text-muted">{xp} XP</span>
    </span>
  );
}

export function LevelProgress({ xp }: { xp: number }) {
  const l = levelFor(xp);
  return (
    <div>
      <div className="flex items-baseline justify-between gap-4">
        <p>
          <span className="text-2xl font-bold">Level {l.level}</span> <span className="text-muted">· {l.title}</span>
        </p>
        <p className="text-sm text-muted tabular-nums">{xp} XP</p>
      </div>
      <div className="mt-3">
        <ProgressBar value={l.progress} label={`Progress to level ${l.level + 1}`} />
      </div>
      <p className="mt-2 text-xs text-muted">
        {l.next === null ? "Maximum level — you've mastered every lesson." : `${l.next - xp} XP to level ${l.level + 1}`}
      </p>
    </div>
  );
}
