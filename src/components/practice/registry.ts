import { lazy } from "react";
import type { PracticeKind } from "@/lib/curriculum/practice";
import type { PracticeKindComponent } from "./types";

/**
 * One component per practice kind. To add a kind, replace its line with
 * `predict: lazy(() => import("./predict"))` and give that file a default export taking `KindProps<"predict">`
 * (see ./types.ts). Components load on demand, so the editor page only pays for the kind it shows.
 */
export const practiceKinds: { [K in PracticeKind]: PracticeKindComponent<K> } = {
  parsons: lazy(() => import("./parsons")),
  blanks: lazy(() => import("./blanks")),
  predict: lazy(() => import("./predict")),
  bug: lazy(() => import("./bug-finder")),
  choice: lazy(() => import("./choice")),
  typed: lazy(() => import("./typed")),
  match: lazy(() => import("./match")),
};
