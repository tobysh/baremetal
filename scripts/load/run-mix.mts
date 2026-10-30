// Load test for /api/run in Test mode: virtual users repeatedly submit the reference solution of a random
// lesson, picked by language from a weighted mix. Plain Node + fetch.
//
//   npx tsx scripts/load/run-mix.mts [--url http://localhost:3100] [--users 5] [--seconds 60]
//                                    [--mix c=50,rust=25,go=15,asm=10] [--shared-ip] [--i-own-this-host]
//
// Each user sends its own X-Forwarded-For (the server must be started with TRUSTED_PROXY_HOPS=1) unless
// --shared-ip simulates a classroom behind one address. Anonymous runs are limited per IP, so raise
// RUN_LIMIT_ANON_IP on the target when testing; 429s are counted separately from 503 (runner busy).
import { getTracks } from "../../src/lib/curriculum/index";

const args = process.argv.slice(2);
const flag = (name: string) => args.includes(`--${name}`);
const opt = (name: string, fallback: string) => {
  const i = args.indexOf(`--${name}`);
  return i >= 0 && args[i + 1] ? args[i + 1] : fallback;
};
const url = opt("url", "http://localhost:3100").replace(/\/$/, "");
const host = new URL(url).hostname;
if (!["localhost", "127.0.0.1", "::1", "[::1]"].includes(host) && !flag("i-own-this-host")) {
  console.error(`Refusing to load-test ${host}. Pass --i-own-this-host if you own it.`);
  process.exit(2);
}
const users = Number(opt("users", "5"));
const seconds = Number(opt("seconds", "60"));
const mix = Object.fromEntries(opt("mix", "c=50,rust=25,go=15,asm=10").split(",").map((p) => [p.split("=")[0], Number(p.split("=")[1])]));

const byLanguage = new Map<string, { id: string; solution: string }[]>();
for (const track of getTracks()) {
  for (const lesson of track.lessons) {
    const ex = lesson.exercise;
    if (!(ex.language in mix)) continue;
    if (!byLanguage.has(ex.language)) byLanguage.set(ex.language, []);
    byLanguage.get(ex.language)!.push({ id: ex.id, solution: ex.solution });
  }
}
const languages = Object.keys(mix).filter((l) => byLanguage.has(l) && mix[l] > 0);
const total = languages.reduce((n, l) => n + mix[l], 0);
const pick = () => {
  let r = Math.random() * total;
  const language = languages.find((l) => (r -= mix[l]) < 0) ?? languages[0];
  const pool = byLanguage.get(language)!;
  return { language, ...pool[Math.floor(Math.random() * pool.length)] };
};

type Sample = { ms: number; status: number | "error"; passed: boolean };
const samples = new Map<string, Sample[]>(languages.map((l) => [l, []]));
const deadline = Date.now() + seconds * 1000;
const sharedIp = `198.51.100.${1 + Math.floor(Math.random() * 250)}`;

async function user(n: number) {
  const ip = flag("shared-ip") ? sharedIp : `198.51.${100 + (n >> 8)}.${(n & 255) + 1}`;
  while (Date.now() < deadline) {
    const job = pick();
    const t0 = performance.now();
    let sample: Sample;
    try {
      const res = await fetch(`${url}/api/run`, {
        method: "POST",
        headers: { "Content-Type": "application/json", Origin: process.env.LOAD_ORIGIN ?? url, "X-Forwarded-For": ip },
        body: JSON.stringify({ exerciseId: job.id, code: job.solution, mode: "test" }),
      });
      const body = res.ok ? ((await res.json()) as { allPassed?: boolean }) : null;
      sample = { ms: performance.now() - t0, status: res.status, passed: !!body?.allPassed };
    } catch {
      sample = { ms: performance.now() - t0, status: "error", passed: false };
      await new Promise((resolve) => setTimeout(resolve, 500)); // a dead server shouldn't be hammered in a tight loop
    }
    samples.get(job.language)!.push(sample);
  }
}

console.log(`Load test: ${users} users for ${seconds}s against ${url}, mix ${JSON.stringify(mix)}${flag("shared-ip") ? ", one shared IP" : ""}`);
await Promise.all(Array.from({ length: users }, (_, n) => user(n)));

const pct = (sorted: number[], p: number) => (sorted.length ? sorted[Math.min(sorted.length - 1, Math.floor((p / 100) * sorted.length))] : NaN);
console.log("\nlang   requests   p50 ms   p95 ms   200 ok   503 busy   429 limit   other   wrong verdict");
for (const [language, list] of samples) {
  const done = list.filter((s) => s.status === 200).map((s) => s.ms).sort((a, b) => a - b);
  const count = (f: (s: Sample) => boolean) => list.filter(f).length;
  const row = [language.padEnd(5), String(list.length).padStart(8), pct(done, 50).toFixed(0).padStart(8), pct(done, 95).toFixed(0).padStart(8),
    String(done.length).padStart(8), String(count((s) => s.status === 503)).padStart(10), String(count((s) => s.status === 429)).padStart(11),
    String(count((s) => s.status !== 200 && s.status !== 503 && s.status !== 429)).padStart(7), String(count((s) => s.status === 200 && !s.passed)).padStart(15)];
  console.log(row.join(" "));
}
const all = [...samples.values()].flat();
console.log(`\n503 rate ${((all.filter((s) => s.status === 503).length / Math.max(1, all.length)) * 100).toFixed(1)}%, error rate ${((all.filter((s) => s.status === "error" || (typeof s.status === "number" && s.status >= 500 && s.status !== 503)).length / Math.max(1, all.length)) * 100).toFixed(1)}%`);
