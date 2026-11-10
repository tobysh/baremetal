import type { Exercise } from "./curriculum/types";

/** How far a pass can be trusted: "driver" verdicts are computed outside the learner's process, "stdout" cases are compared by the runner, "harness" markers are printed by hidden code sharing the learner's process. */
export type GradingTrust = "driver" | "stdout" | "harness";

export function gradingTrust(ex: Pick<Exercise, "mode" | "driver">): GradingTrust {
  if (ex.driver) return "driver";
  return ex.mode === "stdout" ? "stdout" : "harness";
}

/** High-trust rewards (certification, rare achievements) may count only these passes. */
export const isHighTrust = (t: GradingTrust) => t !== "harness";
