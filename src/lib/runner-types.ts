export type RunResult = {
  ok: boolean;
  phase: "compile" | "run";
  compileOutput: string;
  stdout: string;
  stderr: string;
  exitCode: number | null;
  error: string | null;
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
};

