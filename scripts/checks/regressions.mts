// Uses a unique scratch database, temporary secrets and a separate runner process.
import { spawn, type ChildProcess } from "node:child_process";
import { cp, mkdir, mkdtemp, rm, stat, symlink } from "node:fs/promises";
import { createServer } from "node:net";
import { tmpdir } from "node:os";
import path from "node:path";
import { randomBytes } from "node:crypto";
import { setTimeout as delay } from "node:timers/promises";
import { MongoClient } from "mongodb";

const workspace = process.cwd();
const temp = await mkdtemp(path.join(tmpdir(), "bm-regression-"));
const app = path.join(temp, "app");
const socket = path.join(temp, "runner.sock");
const dbName = `bm_regression_${randomBytes(8).toString("hex")}`;
const port = process.env.REGRESSION_PORT ?? await new Promise<string>((resolve, reject) => {
  const reservation = createServer();
  reservation.once("error", reject);
  reservation.listen(0, "127.0.0.1", () => {
    const address = reservation.address();
    if (!address || typeof address === "string") return reject(new Error("No test port available."));
    reservation.close(() => resolve(String(address.port)));
  });
});
const base = `http://127.0.0.1:${port}`;
const env = { ...process.env, NODE_ENV: "production" as const, AUTH_SECRET: randomBytes(48).toString("hex"), TRUSTED_PROXY_HOPS: "1", RUNNER_SOCKET: socket,
  MONGODB_URI: process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", MONGODB_DB: dbName,
  ANTHROPIC_API_KEY: "", SMTP_URL: "smtp://127.0.0.1:9", EMAIL_FROM: "no-reply@regression.test", LEGAL_ENTITY_NAME: "Regression Ltd", LEGAL_ADDRESS: "1 Test Street", LEGAL_CONTACT_EMAIL: "legal@regression.test",
  TUTOR_PROVIDER: "ollama", OLLAMA_HOST: "http://127.0.0.1:9", NEXT_PUBLIC_SITE_URL: base.replace("http:", "https:"), TEST_BASE_URL: base };
const mongo = new MongoClient(env.MONGODB_URI, { serverSelectionTimeoutMS: 5000 });
const children = new Set<ChildProcess>();
let interrupted = false;
const interrupt = () => { interrupted = true; for (const child of children) child.kill("SIGTERM"); };
process.once("SIGINT", interrupt);
process.once("SIGTERM", interrupt);
const start = (args: string[], childEnv: NodeJS.ProcessEnv = env) => {
  if (interrupted) throw new Error("Regression run interrupted.");
  const child = spawn(process.execPath, args, { cwd: app, env: childEnv, stdio: "inherit" });
  children.add(child);
  child.once("exit", () => children.delete(child));
  return child;
};
const run = (args: string[]) => new Promise<void>((resolve, reject) => {
  const child = start(args);
  const timer = setTimeout(() => { child.kill("SIGKILL"); reject(new Error(`Timed out: ${args.join(" ")}`)); }, 300_000);
  child.once("error", (error) => { clearTimeout(timer); reject(error); });
  child.once("exit", (code) => { clearTimeout(timer); if (code === 0) resolve(); else reject(new Error(`Check failed (${code}): ${args.join(" ")}`)); });
});
try {
  await mkdir(app);
  // Snapshot source without secrets, Git state, local databases or shared build output.
  for (const entry of ["src", "content", "data", "public", "scripts", "next.config.ts", "tsconfig.json", "postcss.config.mjs", "package.json", "package-lock.json", "playwright.config.ts"]) {
    await cp(path.join(workspace, entry), path.join(app, entry), { recursive: true });
  }
  await symlink(path.join(workspace, "node_modules"), path.join(app, "node_modules"), "dir");
  await mongo.connect();
  console.log("Building an isolated production snapshot...");
  await run(["node_modules/next/dist/bin/next", "build", "--webpack"]);
  // Tests requiring no web server still use the same isolated database.
  for (const script of ["scripts/auth-crypto-regression.mts", "scripts/tutor/cost-checks.mts", "scripts/tutor/reservation-checks.mts", "scripts/security/quick-fixes.mts", "scripts/security/xp-atomic.mts"]) {
    await run(["--conditions=react-server", "--import", "tsx", script]);
  }
  await run(["--import", "tsx", "scripts/security/markdown.mts"]);
  const runner = start(["--import", "tsx", "scripts/runner-service.mts"], {
    PATH: process.env.PATH, NODE_ENV: "test", RUNNER_SOCKET: socket,
    RUST_TOOLCHAIN_DIR: process.env.RUST_TOOLCHAIN_DIR, GO_TOOLCHAIN_DIR: process.env.GO_TOOLCHAIN_DIR,
  });
  const web = start(["node_modules/next/dist/bin/next", "start", "-p", port, "-H", "127.0.0.1"]);
  let ready = false;
  for (let i = 0; i < 120; i++) {
    if (web.exitCode !== null || runner.exitCode !== null) throw new Error("Regression server exited before readiness.");
    ready = await fetch(base + "/login").then((res) => res.ok).catch(() => false);
    if (ready && await stat(socket).then(() => true).catch(() => false)) break;
    ready = false;
    await delay(500);
  }
  if (!ready) throw new Error("Regression server did not become ready.");
  for (const script of ["scripts/e2e-auth.mts", "scripts/security/api-hardening.mts", "scripts/security/workspace-api.mts"]) {
    await run(["--conditions=react-server", "--import", "tsx", script, base]);
  }
  await run(["node_modules/@playwright/test/cli.js", "test"]);
} finally {
  await Promise.all([...children].map((child) => new Promise<void>((resolve) => {
    const timer = setTimeout(() => child.kill("SIGKILL"), 10_000);
    child.once("exit", () => { clearTimeout(timer); resolve(); });
    child.kill("SIGTERM");
  })));
  try { await mongo.db(dbName).dropDatabase(); } finally { await mongo.close(); }
  const artifacts = path.join(workspace, "test-results", dbName);
  if (await stat(path.join(app, "test-results")).then(() => true).catch(() => false)) {
    await mkdir(path.dirname(artifacts), { recursive: true });
    await cp(path.join(app, "test-results"), artifacts, { recursive: true });
    console.log(`Browser artifacts: ${artifacts}`);
  }
  await rm(temp, { recursive: true, force: true });
  process.off("SIGINT", interrupt);
  process.off("SIGTERM", interrupt);
}
