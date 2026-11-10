import { getTracks } from "../../../src/lib/curriculum";
import { gradingInventory } from "../../../src/lib/grading-inventory";

const args = process.argv.slice(2);
if (args.some((arg) => arg !== "--json")) {
  throw new Error("Usage: npm run audit:grading -- [--json]");
}
const rows = gradingInventory(getTracks());
if (args.includes("--json")) {
  console.log(JSON.stringify(rows, null, 2));
} else {
  console.log("id\ttrack\tlanguage\tkind\ttrust\tsameProcessVerdict");
  for (const row of rows) console.log([row.id, row.track, row.language, row.kind, row.trust, row.sameProcessVerdict].join("\t"));
  console.error(`Published programming assessments: ${rows.length}; driver: ${rows.filter((r) => r.trust === "driver").length}; stdout: ${rows.filter((r) => r.trust === "stdout").length}; same-process harness: ${rows.filter((r) => r.sameProcessVerdict).length}.`);
}
