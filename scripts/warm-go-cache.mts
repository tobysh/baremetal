// Builds the Go standard library into $GO_CACHE_SEED, inside the same sandbox the runner uses.
// Run at deploy time and after upgrading Go:
//   mkdir -p /var/cache/baremetal/gocache && GO_CACHE_SEED=/var/cache/baremetal/gocache npx tsx scripts/warm-go-cache.mts
import { existsSync } from "node:fs";
import { warmGoCache } from "../src/lib/runner-engine";

if (!process.env.GO_CACHE_SEED || !existsSync(process.env.GO_CACHE_SEED)) {
  console.error("GO_CACHE_SEED must name an existing directory.");
  process.exit(1);
}
const started = Date.now();
const r = await warmGoCache();
console.log(r.code === 0 ? `Go cache warmed in ${((Date.now() - started) / 1000).toFixed(1)} s` : `Warm-up failed (${r.code}):\n${r.stderr}${r.stdout}`);
process.exit(r.code === 0 ? 0 : 1);
