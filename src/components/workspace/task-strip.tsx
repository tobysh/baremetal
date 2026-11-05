import { MiniMarkdown } from "./mini-markdown";

/** The objective in one line, with the full task one click away, so the learner knows what to type. */
export function TaskStrip({ task }: { task: string }) {
  const first = task.split("\n").map((l) => l.trim()).find((l) => l && !l.startsWith("#") && !l.startsWith("```")) ?? "";
  const objective = first.replace(/[`*_]/g, "");
  return (
    <details className="group mb-2 shrink-0 panel px-4 text-sm">
      <summary className="flex min-h-10 cursor-pointer list-none items-center gap-2 [&::-webkit-details-marker]:hidden">
        <span className="shrink-0 font-semibold">Task</span>
        <span className="min-w-0 flex-1 truncate text-text-2 group-open:hidden">{objective}</span>
        <span className="ml-auto shrink-0 text-xs text-accent underline underline-offset-2 group-open:hidden">View task</span>
        <span className="ml-auto hidden shrink-0 text-xs text-accent underline underline-offset-2 group-open:inline">Hide</span>
      </summary>
      <div className="max-h-48 overflow-y-auto pb-3 pt-1"><MiniMarkdown text={task} /></div>
    </details>
  );
}

