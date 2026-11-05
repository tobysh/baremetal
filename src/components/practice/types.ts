import type { ComponentType } from "react";
import type { Language } from "@/lib/curriculum/types";
import type { PracticeKind, PracticeSubmission, PublicPractice } from "@/lib/curriculum/practice";

/**
 * What the shell hands to every practice kind component. The kind owns the interaction; the shell owns the
 * "Check answer" button, the request to POST /api/practice, feedback and XP.
 */
export type KindProps<K extends PracticeKind = PracticeKind> = {
  practice: Extract<PublicPractice, { kind: K }>;
  /** Source language of the lesson, for syntax colouring. */
  language: Language;
  /** True while a check is in flight; the kind should ignore input. */
  locked: boolean;
  /** Call with the current submission whenever it changes, or null while the answer is incomplete. The shell clears old feedback on every call. */
  onAnswer: (submission: PracticeSubmission | null) => void;
  /** Counts wrong checks, so a kind can reset or shake something. Starts at 0. */
  attempt: number;
};

export type PracticeKindComponent<K extends PracticeKind = PracticeKind> = ComponentType<KindProps<K>>;
