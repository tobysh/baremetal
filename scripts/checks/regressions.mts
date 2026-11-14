// Uses a unique scratch database, temporary secrets and a separate runner process.
import { execFile, spawn, type ChildProcess } from "node:child_process";
import { cp, mkdir, mkdtemp, readFile, readdir, rm, stat, symlink } from "node:fs/promises";
import { createServer as createNetServer } from "node:net";
import { createServer as createHttpsServer, type Server as HttpsServer } from "node:https";
import { request as httpRequest } from "node:http";
import { promisify } from "node:util";
import { tmpdir } from "node:os";
import path from "node:path";
import { createHash, randomBytes, X509Certificate } from "node:crypto";
import { setTimeout as delay } from "node:timers/promises";
import { MongoClient } from "mongodb";

const workspace = process.cwd();
// Allow a clean lockfile install in /tmp without replacing the developer's active dependencies.
const dependencies = path.resolve(process.env.REGRESSION_NODE_MODULES ?? path.join(workspace, "node_modules"));
const temp = await mkdtemp(path.join(tmpdir(), "bm-regression-"));
const app = path.join(temp, "app");
const socket = path.join(temp, "runner.sock");
const dbName = `bm_regression_${randomBytes(8).toString("hex")}`;
const unusedPort = () => new Promise<string>((resolve, reject) => {
  const reservation = createNetServer();
  reservation.once("error", reject);
  reservation.listen(0, "127.0.0.1", () => {
    const address = reservation.address();
    if (!address || typeof address === "string") return reject(new Error("No test port available."));
    reservation.close(() => resolve(String(address.port)));
  });
});
const port = process.env.REGRESSION_PORT ?? await unusedPort();
const appPort = await unusedPort();
const base = `https://127.0.0.1:${port}`;
const internalBase = `http://127.0.0.1:${appPort}`;
const certificate = path.join(temp, "localhost.crt");
const privateKey = path.join(temp, "localhost.key");
const env = { ...process.env, NODE_ENV: "production" as const, AUTH_SECRET: randomBytes(48).toString("hex"), TRUSTED_PROXY_HOPS: "1", RUNNER_SOCKET: socket,
  MONGODB_URI: process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017", MONGODB_DB: dbName,
  ANTHROPIC_API_KEY: "", SMTP_URL: "smtp://127.0.0.1:9", EMAIL_FROM: "no-reply@regression.test", LEGAL_ENTITY_NAME: "Regression Ltd", LEGAL_ADDRESS: "1 Test Street", LEGAL_CONTACT_EMAIL: "legal@regression.test",
  TUTOR_PROVIDER: "ollama", OLLAMA_HOST: "http://127.0.0.1:9", NEXT_PUBLIC_SITE_URL: base, TEST_BASE_URL: base,
  NODE_EXTRA_CA_CERTS: certificate, BM_REGRESSION_HTTPS: "1", BM_REGRESSION_TLS_SPKI: "" };
const mongo = new MongoClient(env.MONGODB_URI, { serverSelectionTimeoutMS: 5000 });
const children = new Set<ChildProcess>();
let interrupted = false;
let tlsProxy: HttpsServer | undefined;
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
  // Exercise complete-origin protections over actual HTTPS. Trust this ephemeral
  // certificate only in child checks; never disable TLS verification globally.
  await promisify(execFile)("openssl", ["req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", privateKey,
    "-out", certificate, "-days", "1", "-subj", "/CN=127.0.0.1", "-addext", "subjectAltName=IP:127.0.0.1"], { timeout: 30_000 });
  env.BM_REGRESSION_TLS_SPKI = createHash("sha256").update(new X509Certificate(await readFile(certificate)).publicKey.export({ type: "spki", format: "der" })).digest("base64");
  // Snapshot source without secrets, Git state, local databases or shared build output.
  for (const entry of ["src", "content", "data", "public", "scripts", "tests", "eslint.config.mjs", "next.config.ts", "tsconfig.json", "postcss.config.mjs", "package.json", "package-lock.json", "playwright.config.ts"]) {
    await cp(path.join(workspace, entry), path.join(app, entry), { recursive: true });
  }
  await symlink(dependencies, path.join(app, "node_modules"), "dir");
  await mongo.connect();
  console.log("Building an isolated production snapshot...");
  await run(["node_modules/next/dist/bin/next", "build", "--webpack"]);
  await run(["node_modules/eslint/bin/eslint.js"]);
  // Run against the snapshot's dependency install too, including any clean lockfile override.
  const unitFiles = (await readdir(path.join(app, "tests/unit"))).filter((file) => file.endsWith(".test.mts")).sort();
  await run(["--conditions=react-server", "--import", "tsx", "--test", ...unitFiles.map((file) => path.join("tests/unit", file))]);
  // Tests requiring no web server still use the same isolated database.
  for (const script of ["scripts/auth-crypto-regression.mts", "scripts/tutor/cost-checks.mts", "scripts/tutor/reservation-checks.mts", "scripts/tutor/abort-checks.mts", "scripts/tutor/stream-guard-checks.mts", "scripts/checks/review.mts", "scripts/security/seals.mts", "scripts/security/accounting-health.mts", "scripts/security/backup-restore.mts", "scripts/security/integrity/assembly-driver.mts", "scripts/security/quick-fixes.mts", "scripts/security/xp-atomic.mts"]) {
    await run(["--conditions=react-server", "--import", "tsx", script]);
  }
  await run(["--import", "tsx", "scripts/security/markdown.mts"]);
  const runner = start(["--import", "tsx", "scripts/runner-service.mts"], {
    PATH: process.env.PATH, NODE_ENV: "test", RUNNER_SOCKET: socket,
    RUST_TOOLCHAIN_DIR: process.env.RUST_TOOLCHAIN_DIR, GO_TOOLCHAIN_DIR: process.env.GO_TOOLCHAIN_DIR,
  });
  const web = start(["node_modules/next/dist/bin/next", "start", "-p", appPort, "-H", "127.0.0.1"]);
  tlsProxy = createHttpsServer({ key: await readFile(privateKey), cert: await readFile(certificate) }, (req, res) => {
    const upstream = httpRequest(internalBase, {
      // Body-limit probes intentionally abort uploads; never reuse their upstream sockets.
      agent: false,
      path: req.url ?? "/",
      method: req.method,
      headers: { ...req.headers, "x-forwarded-proto": "https", "x-forwarded-for": req.headers["x-forwarded-for"] ?? "127.0.0.1" },
    }, (reply) => {
      res.writeHead(reply.statusCode ?? 502, reply.headers);
      reply.pipe(res);
    });
    // Preserve upstream connection cancellation (including bounded-body rejection)
    // as a transport failure instead of inventing an application 502 response.
    upstream.on("error", (error) => res.destroy(error));
    req.on("aborted", () => upstream.destroy());
    res.on("close", () => { if (!res.writableFinished) upstream.destroy(); });
    req.pipe(upstream);
  });
  await new Promise<void>((resolve, reject) => {
    tlsProxy!.once("error", reject);
    tlsProxy!.listen(Number(port), "127.0.0.1", resolve);
  });
  let ready = false;
  for (let i = 0; i < 120; i++) {
    if (web.exitCode !== null || runner.exitCode !== null) throw new Error("Regression server exited before readiness.");
    ready = await fetch(internalBase + "/login").then((res) => res.ok).catch(() => false);
    if (ready && await stat(socket).then(() => true).catch(() => false)) break;
    ready = false;
    await delay(500);
  }
  if (!ready) throw new Error("Regression server did not become ready.");
  await run(["--import", "tsx", "scripts/security/csp-check.mts", base]);
  for (const script of ["scripts/e2e-auth.mts", "scripts/security/api-hardening.mts", "scripts/security/workspace-api.mts"]) {
    await run(["--conditions=react-server", "--import", "tsx", script, base]);
  }
  await run(["node_modules/@playwright/test/cli.js", "test"]);
} finally {
  if (tlsProxy) {
    tlsProxy.closeAllConnections();
    await new Promise<void>((resolve) => tlsProxy!.close(() => resolve()));
  }
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
