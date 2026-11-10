import { readdir, readFile } from "node:fs/promises";
import path from "node:path";
import { inlineScripts, scriptHash } from "./csp";

/** A partial or missing build cannot safely supply the page script allowlist. */
export async function loadShellScriptHashes(root: string): Promise<string[]> {
  const hashes = new Set<string>();
  const walk = async (dir: string): Promise<void> => {
    for (const entry of await readdir(dir, { withFileTypes: true })) {
      const file = path.join(dir, entry.name);
      if (entry.isDirectory()) await walk(file);
      else if (entry.name.endsWith(".html")) {
        for (const body of inlineScripts(await readFile(file, "utf8"))) hashes.add(scriptHash(body));
      }
    }
  };
  await walk(root);
  if (!hashes.size) throw new Error("No executable scripts found in prerendered shells");
  return [...hashes].sort();
}
