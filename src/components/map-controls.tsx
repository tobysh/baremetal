"use client";

/** Expand or collapse every course group in the map. Without JS the individual disclosures still work. */
export function MapControls({ targetId }: { targetId: string }) {
  const set = (open: boolean) => document.querySelectorAll<HTMLDetailsElement>(`#${targetId} details`).forEach((d) => { d.open = open; });
  const cls = "min-h-9 rounded-md px-2.5 text-sm font-medium text-muted hover:text-accent";
  return (
    <div className="flex items-center gap-1" role="group" aria-label="Course map">
      <button type="button" onClick={() => set(true)} className={cls}>Expand all</button>
      <button type="button" onClick={() => set(false)} className={cls}>Collapse all</button>
    </div>
  );
}
