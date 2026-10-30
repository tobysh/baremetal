// Direct engine entrypoint for curriculum and sandbox verification scripts.
// Web handlers must use runner-client; production never executes locally.
export { RunnerBusyError, withRunnerRequest, runProgram, testExercise, redactHarnessDiagnostics, cleanupStaleWorkdirs, runnerStatus } from "./runner-engine";
export type { RunResult, TestResult, TestCaseResult } from "./runner-types";
