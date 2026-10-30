import { test, expect, type Page } from "@playwright/test";
import { MongoClient, ObjectId } from "mongodb";
import { createHash, randomBytes } from "node:crypto";
import { getTracks } from "../../src/lib/curriculum";

const base = process.env.TEST_BASE_URL ?? "http://127.0.0.1:3100";
const dbName = process.env.MONGODB_DB;
if (!dbName?.startsWith("bm_regression_")) throw new Error("Browser tests require the scratch database created by npm run test:regressions.");
const mongo = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017");
const db = mongo.db(dbName);
const userId = new ObjectId(), adminId = new ObjectId();
const userToken = randomBytes(32).toString("hex"), adminToken = randomBytes(32).toString("hex");
const tracks = getTracks();
const zero = tracks.find((track) => track.slug === "first-steps")!;
const go = tracks.find((track) => track.slug === "go-foundations")!;
const login = async (page: Page, admin = false) => page.context().addCookies([{ name: "bm_session", value: admin ? adminToken : userToken, url: base, httpOnly: true, sameSite: "Strict" }]);
const edit = async (page: Page, code: string) => {
  const editor = page.getByRole("textbox", { name: /code editor/ });
  await editor.click();
  await page.keyboard.press("ControlOrMeta+a");
  await page.keyboard.insertText(code);
};

test.beforeAll(async () => {
  await mongo.connect();
  for (const [id, token, role] of [[userId, userToken, "user"], [adminId, adminToken, "admin"]] as const) {
    await db.collection("users").insertOne({ _id: id, name: "Browser fixture", email: `browser-${id}@example.test`, passwordHash: "unused", emailVerified: true, experience: "none", role, xp: 0, sessionVersion: 0, createdAt: new Date("2026-01-01"), ...(role === "admin" ? { twoFactor: { enabled: true, secretEnc: "unused-fixture", backupCodeHashes: [] } } : {}) });
    await db.collection("sessions").insertOne({ userId: id, tokenHash: createHash("sha256").update(token).digest("hex"), sessionVersion: 0, createdAt: new Date(), expiresAt: new Date(Date.now() + 3_600_000) });
  }
});
test.afterAll(async () => { await mongo.close(); });
test.beforeEach(async () => { await db.collection("progress").deleteMany({ userId }); });

test("anonymous recommendations and small-screen catalogue", async ({ page }) => {
  await page.setViewportSize({ width: 375, height: 812 });
  await page.goto("/learn");
  await expect(page.getByRole("heading", { name: "Your recommended starting point" })).toBeVisible();
  await expect(page.getByRole("link", { name: "Start first lesson" })).toHaveAttribute("href", "/learn/first-steps/hello-world");
  await expect(page.getByText(zero.outcomes[0])).toBeVisible();
  expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
  await page.screenshot({ path: "test-results/catalogue-375.png", fullPage: true });
});

test("a Go pass continues Go despite unfinished beginner lessons", async ({ page }) => {
  await login(page);
  await db.collection("progress").insertOne({ userId, exerciseId: go.lessons[0].exercise.id, trackSlug: go.slug, status: "passed", code: "", attempts: 1, startedAt: new Date(), passedAt: new Date(), updatedAt: new Date() });
  await page.goto("/dashboard");
  await expect(page.getByRole("heading", { level: 1 })).toHaveText(go.lessons[1].title);
  await expect(page.getByRole("link", { name: "Open lesson" })).toHaveAttribute("href", `/learn/${go.slug}/${go.lessons[1].slug}`);
});

test("anonymous final lesson offers the C course transition", async ({ page }) => {
  await page.goto(`/learn/first-steps/${zero.lessons.at(-1)!.slug}`);
  await page.route("**/api/run", (route) => route.fulfill({ json: { kind: "test", compiled: true, compileOutput: "", results: [{ name: "example", passed: true }], allPassed: true, output: "", error: null, saved: false, reward: null } }));
  await page.getByRole("button", { name: /^Test/ }).click();
  await expect(page.getByRole("link", { name: /Next course:/ })).toHaveAttribute("href", "/learn/c-data/arrays");
});

test("failed saves can be retried without losing the editor draft", async ({ page }) => {
  await login(page);
  await page.goto("/learn/first-steps/hello-world");
  let fail = true;
  const saved: string[] = [];
  await page.route("**/api/progress", async (route) => {
    if (route.request().method() !== "PUT") return route.continue();
    if (fail) return route.fulfill({ status: 503, json: { error: "offline" } });
    saved.push(route.request().postDataJSON().code);
    return route.fulfill({ json: { ok: true } });
  });
  const draft = '#include <stdio.h>\nint main(void){puts("recovery draft");}\n';
  await edit(page, draft);
  await expect(page.getByRole("button", { name: "Retry save" })).toBeVisible();
  await expect(page.getByRole("textbox", { name: /code editor/ })).toContainText("recovery draft");
  fail = false;
  await page.getByRole("button", { name: "Retry save" }).click();
  await expect(page.getByText("Saved", { exact: true })).toBeVisible();
  expect(saved.at(-1)).toBe(draft);
});

test("stale saves cannot display Saved while a newer draft is unsaved", async ({ page }) => {
  await login(page);
  await page.goto("/learn/first-steps/hello-world");
  let release!: () => void;
  const gate = new Promise<void>((resolve) => { release = resolve; });
  let accepted!: () => void;
  const started = new Promise<void>((resolve) => { accepted = resolve; });
  const saves: string[] = [];
  await page.route("**/api/progress", async (route) => {
    if (route.request().method() !== "PUT") return route.continue();
    saves.push(route.request().postDataJSON().code);
    if (saves.length === 1) { accepted(); await gate; }
    await route.fulfill({ json: { ok: true } });
  });
  await edit(page, "first draft");
  await started;
  await edit(page, "latest draft");
  release();
  await expect(page.getByText("Saved", { exact: true })).not.toBeVisible();
  await expect.poll(() => saves.length).toBe(2);
  await expect(page.getByText("Saved", { exact: true })).toBeVisible();
  expect(saves).toEqual(["first draft", "latest draft"]);
});

test("navigation aborts execution and the next lesson stays clean", async ({ page }) => {
  await page.goto("/learn/first-steps/hello-world");
  let started!: () => void;
  const sent = new Promise<void>((resolve) => { started = resolve; });
  const failures: string[] = [];
  page.on("requestfailed", (request) => { if (request.url().endsWith("/api/run")) failures.push(request.failure()?.errorText ?? ""); });
  await page.route("**/api/run", async (route) => { started(); await new Promise((resolve) => setTimeout(resolve, 2000)); await route.fulfill({ json: { kind: "run", stdout: "STALE RESULT" } }).catch(() => {}); });
  await page.getByRole("button", { name: /^Run/ }).click();
  await sent;
  await page.getByRole("navigation", { name: "Lesson navigation" }).getByRole("link", { name: new RegExp(zero.lessons[1].title) }).click();
  await expect(page.getByRole("heading", { level: 1 })).toHaveText(zero.lessons[1].title);
  await expect.poll(() => failures.length).toBeGreaterThan(0);
  await expect(page.getByText("STALE RESULT")).not.toBeVisible();
});

test("keyboard tabs, editor escape and service-unavailable recovery", async ({ page }) => {
  await page.setViewportSize({ width: 375, height: 812 });
  await page.goto("/learn/first-steps/hello-world");
  await page.getByRole("button", { name: "Reject analytics" }).click();
  await page.getByRole("tab", { name: "Read lesson" }).focus();
  await page.keyboard.press("ArrowRight");
  await expect(page.getByRole("tab", { name: "Code & feedback" })).toBeFocused();
  const editor = page.getByRole("textbox", { name: /code editor/ });
  await editor.click();
  await page.keyboard.press("Escape");
  await page.keyboard.press("Tab");
  await expect(editor).not.toBeFocused();
  const output = page.getByRole("tab", { name: "Output", exact: true });
  await output.focus();
  await page.keyboard.press("ArrowRight");
  await expect(page.getByRole("tab", { name: /Hints/ })).toBeFocused();
  await expect(page.getByRole("tab", { name: /Hints/ })).toHaveAttribute("aria-selected", "true");
  await page.route("**/api/run", (route) => route.fulfill({ status: 503, json: { error: "unavailable" } }));
  await page.getByRole("button", { name: /^Run/ }).click();
  await expect(page.getByRole("tabpanel", { name: "Output", exact: true }).getByRole("alert")).toContainText("Your draft is still here");
  expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
  await page.screenshot({ path: "test-results/workspace-375.png", fullPage: true });
});

test("cohort reports require an administrator", async ({ page }) => {
  await login(page);
  await page.goto("/admin/stuck");
  await expect(page).toHaveURL(/\/dashboard$/);
  await expect(page.getByRole("heading", { level: 1 })).toHaveText(zero.lessons[0].title);
  await expect(page.getByRole("heading", { name: "Learning cohorts" })).toHaveCount(0);
  await login(page, true);
  await page.goto("/admin/stuck");
  await expect(page.getByRole("heading", { name: "Learning cohorts" })).toBeVisible();
  await expect(page.getByText(/not login retention/)).toBeVisible();
});

test("failed account drafts survive reload and stay account-scoped", async ({ page }) => {
  await login(page);
  await page.goto("/learn/first-steps/hello-world");
  let fail = true;
  await page.route("**/api/progress", (route) => route.request().method() !== "PUT" ? route.continue() : route.fulfill({ status: fail ? 503 : 200, json: fail ? { error: "offline" } : { ok: true } }));
  await edit(page, "recovered account draft");
  await expect(page.getByRole("button", { name: "Retry save" })).toBeVisible();
  page.once("dialog", (dialog) => dialog.accept());
  await page.reload();
  await expect(page.getByRole("textbox", { name: /code editor/ })).toContainText("recovered account draft");
  await expect(page.getByRole("button", { name: "Save recovered draft" })).toBeVisible();
  fail = false;
  await page.getByRole("button", { name: "Save recovered draft" }).click();
  await expect(page.getByText("Saved", { exact: true })).toBeVisible();
  await login(page, true);
  await page.reload();
  await expect(page.getByRole("textbox", { name: /code editor/ })).not.toContainText("recovered account draft");
});

test("lesson navigation resets completed state", async ({ page }) => {
  await login(page);
  await db.collection("progress").insertOne({ userId, exerciseId: zero.lessons[0].exercise.id, trackSlug: zero.slug, status: "passed", code: zero.lessons[0].exercise.solution, attempts: 1, startedAt: new Date(), passedAt: new Date(), updatedAt: new Date() });
  await page.goto("/learn/first-steps/hello-world");
  await expect(page.getByText("You've completed this lesson.")).toBeVisible();
  await page.getByRole("navigation", { name: "Lesson navigation" }).getByRole("link", { name: new RegExp(zero.lessons[1].title) }).click();
  await expect(page.getByRole("heading", { level: 1 })).toHaveText(zero.lessons[1].title);
  await expect(page.getByText("You've completed this lesson.")).not.toBeVisible();
});

test("celebration contains focus and returns it to the Test action", async ({ page }) => {
  await page.goto("/learn/first-steps/hello-world");
  await page.route("**/api/run", (route) => route.fulfill({ json: {
    kind: "test", compiled: true, compileOutput: "", results: [], allPassed: true, output: "", error: null, saved: false, reward: null,
    trackComplete: { slug: zero.slug, title: zero.title, lessons: zero.lessons.length, unaided: 0, challenges: 0, next: { slug: "c-data", title: "C Data" } },
  } }));
  const action = page.getByRole("button", { name: /^Test/ });
  await action.click();
  const dialog = page.getByRole("dialog");
  await expect(dialog).toBeVisible();
  await expect(dialog.getByRole("link", { name: /Next course:/ })).toBeFocused();
  await page.keyboard.press("Shift+Tab");
  expect(await dialog.evaluate((element) => element.contains(document.activeElement))).toBe(true);
  await page.keyboard.press("Escape");
  await expect(dialog).not.toBeVisible();
  await expect(action).toBeFocused();
});

test("account autosave works when browser storage is unavailable", async ({ page }) => {
  await login(page);
  await page.addInitScript(() => {
    Storage.prototype.getItem = () => { throw new DOMException("Storage disabled", "SecurityError"); };
    Storage.prototype.setItem = () => { throw new DOMException("Storage disabled", "SecurityError"); };
  });
  await page.goto("/learn/first-steps/hello-world");
  await page.route("**/api/progress", (route) => route.request().method() !== "PUT" ? route.continue() : route.fulfill({ json: { ok: true } }));
  await edit(page, "server-saved draft");
  await expect(page.getByText("Saved", { exact: true })).toBeVisible();
  await expect(page.getByRole("button", { name: "Retry save" })).toHaveCount(0);
});

test("large valid drafts are not restricted by the browser keepalive budget", async ({ page }) => {
  await login(page);
  await page.goto("/learn/first-steps/hello-world");
  let savedLength = 0;
  await page.route("**/api/progress", (route) => {
    if (route.request().method() !== "PUT") return route.continue();
    savedLength = route.request().postDataJSON().code.length;
    return route.fulfill({ json: { ok: true } });
  });
  const code = "//" + "a".repeat(65530);
  await edit(page, code);
  await expect(page.getByText("Saved", { exact: true })).toBeVisible();
  expect(savedLength).toBe(code.length);
});
