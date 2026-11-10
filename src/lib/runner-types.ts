export type RunResult = {
  ok: boolean;
  phase: "compile" | "run";
  compileOutput: string;
  stdout: string;
  stderr: string;
  exitCode: number | null;
  error: string | null;
  /** True when a time limit stopped the compiler or the program (as opposed to a crash or a wrong answer). */
  timedOut?: boolean;
  durationMs: number;
};

export type TestCaseResult = { name: string; passed: boolean; message?: string; expected?: string; actual?: string };
export type TestResult = {
  compiled: boolean;
  compileOutput: string;
  results: TestCaseResult[];
  allPassed: boolean;
  output: string;
  error: string | null;
  /** True when a time limit stopped the compiler or a test run. */
  timedOut?: boolean;
};

