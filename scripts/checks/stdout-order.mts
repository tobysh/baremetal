// A plain stdout case that follows a `set` case must run the learner's own code, not the variant
// that the `set` case compiled over ./prog. Needs gcc and bubblewrap, like verify:content.
import { testExercise } from "../../src/lib/runner";
import type { Exercise } from "../../src/lib/curriculum/types";

const code = '#include <stdio.h>\nint main(void) {\n    int minutes = 275;\n    printf("%d\\n", minutes);\n    return 0;\n}\n';
const ex: Exercise = {
  id: "check/stdout-order",
  language: "c",
  task: "",
  starter: code,
  solution: code,
  mode: "stdout",
  hints: [],
  cases: [
    { name: "variant first", expected: "500", set: { minutes: "500" } },
    { name: "plain afterwards", expected: "275" },
    { name: "another variant", expected: "0", set: { minutes: "0" } },
    { name: "plain again", expected: "275" },
  ],
};
const r = await testExercise(ex, code);
for (const t of r.results) console.log(`  ${t.passed ? "✓" : "✗"} ${t.name}${t.passed ? "" : ` — ${t.message}`}`);
console.log(r.allPassed ? "\nAll stdout-order checks passed." : "\nA plain case ran a modified binary.");
process.exit(r.allPassed ? 0 : 1);
