// Unit-style checks for setInitialValues (the `set` variants in cases.json): it must rewrite the
// real declaration, never a comment or a string that mentions the same name first.
import { setInitialValues } from "../../src/lib/runner-engine";

let failures = 0;
function expect(name: string, code: string, set: Record<string, string>, want: string | null) {
  const got = setInitialValues(code, set);
  const ok = got === want;
  console.log(`${ok ? "  ✓" : "  ✗"} ${name}`);
  if (!ok) {
    failures++;
    console.log(`      want: ${JSON.stringify(want)}\n      got:  ${JSON.stringify(got)}`);
  }
}

expect("plain C declaration", "int minutes = 275;", { minutes: "500" }, "int minutes = 500;");
expect("line comment before the declaration", "// minutes = the total time\nint minutes = 275;", { minutes: "500" }, "// minutes = the total time\nint minutes = 500;");
expect("block comment before the declaration", "/* minutes = 3; */\nint minutes = 275;", { minutes: "9" }, "/* minutes = 3; */\nint minutes = 9;");
expect("string literal before the declaration", 'printf("minutes = %d\\n", 1);\nint minutes = 275;', { minutes: "0" }, 'printf("minutes = %d\\n", 1);\nint minutes = 0;');
expect("char literal and lifetime do not confuse the scanner", "let q = '\"'; let r: &'static str = \"x\";\nlet n: i32 = 3;", { n: "-4" }, "let q = '\"'; let r: &'static str = \"x\";\nlet n: i32 = -4;");
expect("Rust let with a type annotation", "let n: i32 = 3;", { n: "10" }, "let n: i32 = 10;");
expect("Go short declaration", "x := 3", { x: "7" }, "x := 7");
expect("Go raw string before the declaration", "var s = `x = 1`\nx := 3", { x: "8" }, "var s = `x = 1`\nx := 8");
expect("comma declarations", "int a = 7, b = 8;", { a: "1", b: "2" }, "int a = 1, b = 2;");
expect("quoted values are kept intact", 'const char *name = "Ada, Lovelace";', { name: '"Grace, Hopper"' }, 'const char *name = "Grace, Hopper";');
expect("a name only in a comment is not found", "// int minutes = 3;\nint other = 1;", { minutes: "5" }, null);
expect("a missing variable is reported", "int a = 1;", { z: "2" }, null);
console.log(failures ? `\n${failures} check(s) failed.` : "\nAll set-value checks passed.");
process.exit(failures ? 1 : 0);
