import { test, expect, type Page } from "@playwright/test";
import { MongoClient, ObjectId } from "mongodb";
import { createHash, randomBytes } from "node:crypto";
import { getTracks } from "../../src/lib/curriculum";

const base = process.env.TEST_BASE_URL ?? "http://127.0.0.1:3100";
const dbName = process.env.MONGODB_DB;
if (!dbName?.startsWith("bm_regression_")) throw new Error("Browser tests require the scratch database created by npm run test:regressions.");
const mongo = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017");
const db = mongo.db(dbName);
const userId = new ObjectId();
const token = randomBytes(32).toString("hex");
const first = getTracks().find((track) => track.slug === "first-steps")!;
const lesson = "/learn/first-steps/hello-world";
const reject = (page: Page) => page.getByRole("button", { name: "Reject analytics" }).click().catch(() => {});
const output = (page: Page) => page.getByRole("tabpanel", { name: "Output", exact: true });

test.beforeAll(async () => {
  await mongo.connect();
  await db.collection("users").insertOne({ _id: userId, name: "Discovery fixture", email: `discovery-${userId}@example.test`, passwordHash: "unused", emailVerified: true, experience: "none", role: "user", xp: 0, sessionVersion: 0, createdAt: new Date("2026-01-01") });
  await db.collection("sessions").insertOne({ userId, tokenHash: createHash("sha256").update(token).digest("hex"), sessionVersion: 0, createdAt: new Date(), expiresAt: new Date(Date.now() + 3_600_000) });
});
test.afterAll(async () => { await mongo.close(); });

test.describe("search", () => {
  test("a concept query finds labelled lessons and courses; Escape closes", async ({ page }) => {
    await page.goto("/learn");
    await reject(page);
    await page.keyboard.press("ControlOrMeta+k");
    const dialog = page.getByRole("dialog", { name: "Search" });
    await expect(dialog).toBeVisible();
    await dialog.getByRole("combobox").fill("pointers");
    const options = dialog.getByRole("option");
    await expect(options.filter({ hasText: "Lesson" }).first()).toBeVisible();
    await expect(options.filter({ hasText: "Course" }).first()).toBeVisible();
    await page.keyboard.press("ArrowDown");
    await expect(options.nth(1)).toHaveAttribute("aria-selected", "true");
    await page.keyboard.press("Escape");
    await expect(dialog).not.toBeVisible();
  });

  test("learning paths are labelled", async ({ page }) => {
    await page.goto("/learn");
    await reject(page);
    await page.keyboard.press("ControlOrMeta+k");
    const dialog = page.getByRole("dialog", { name: "Search" });
    await dialog.getByRole("combobox").fill("security");
    await expect(dialog.getByRole("option").filter({ hasText: "Learning path" }).first()).toBeVisible();
  });

  test("no results offers a way back to the course list", async ({ page }) => {
    await page.goto("/learn");
    await reject(page);
    await page.keyboard.press("ControlOrMeta+k");
    const dialog = page.getByRole("dialog", { name: "Search" });
    await dialog.getByRole("combobox").fill("zzqxj");
    await expect(dialog.getByRole("option")).toHaveCount(0);
    await expect(dialog.getByRole("link", { name: "browse all courses" })).toHaveAttribute("href", "/learn");
  });
});

test("the next unfinished lesson is marked Up next with text and aria-current", async ({ page }) => {
  await page.context().addCookies([{ name: "bm_session", value: token, url: base, httpOnly: true, sameSite: "Strict" }]);
  await db.collection("progress").deleteMany({ userId });
  await db.collection("progress").insertOne({ userId, exerciseId: first.lessons[0].exercise.id, trackSlug: first.slug, status: "passed", code: "", attempts: 1, startedAt: new Date(), passedAt: new Date(), updatedAt: new Date() });
  await page.goto(`/learn/${first.slug}`);
  const next = page.locator(`ol a[href="/learn/${first.slug}/${first.lessons[1].slug}"]`);
  await expect(next).toHaveAttribute("aria-current", "step");
  await expect(next).toContainText("Up next");
  await expect(page.getByRole("link", { name: new RegExp(first.lessons[0].title) }).first()).toContainText("Completed");
  await expect(page.locator('a[aria-current="step"]')).toHaveCount(1);
});

test.describe("run failures", () => {
  const cases: [string, (page: Page) => Promise<unknown>, string, boolean][] = [
    ["network failure", (page) => page.route("**/api/run", (route) => route.abort("failed")), "Couldn't reach Baremetal", true],
    ["runner busy", (page) => page.route("**/api/run", (route) => route.fulfill({ status: 503, json: { error: "busy" } })), "Code runner busy or unavailable", true],
    ["rate limited", (page) => page.route("**/api/run", (route) => route.fulfill({ status: 429, json: { error: "Run limit reached. Create a free account for more runs." } })), "Run limit reached", false],
    ["server error", (page) => page.route("**/api/run", (route) => route.fulfill({ status: 500, json: { error: "x" } })), "Server error", true],
  ];
  for (const [name, setup, title, retry] of cases) {
    test(name, async ({ page }) => {
      await page.goto(lesson);
      await reject(page);
      await setup(page);
      await page.getByRole("button", { name: /^Test/ }).click();
      const alert = output(page).getByRole("alert");
      await expect(alert).toContainText(title);
      await expect(alert).toContainText("Your draft is still here");
      await expect(alert.getByRole("button", { name: /^Try (testing|running) again/ })).toHaveCount(retry ? 1 : 0);
    });
  }

  test("offline is distinguished from a network failure, and retry works once back", async ({ page, context }) => {
    await page.goto(lesson);
    await reject(page);
    await context.setOffline(true);
    await page.getByRole("button", { name: /^Test/ }).click();
    await expect(output(page).getByRole("alert")).toContainText("You're offline");
    await context.setOffline(false);
    await output(page).getByRole("button", { name: "Try testing again" }).click();
    await expect(output(page).getByRole("alert")).toHaveCount(0);
  });

  test("a time limit is reported as Timed out, not as a crash", async ({ page }) => {
    await page.goto(lesson);
    await reject(page);
    await page.route("**/api/run", (route) => route.fulfill({ json: { kind: "run", ok: false, phase: "run", compileOutput: "", stdout: "", stderr: "", exitCode: null, error: "Your program ran for more than 5 seconds and was stopped.", timedOut: true, durationMs: 5000 } }));
    await page.getByRole("button", { name: /^Run/ }).click();
    await expect(output(page).getByRole("heading", { name: /Timed out/ })).toBeVisible();
    await page.unroute("**/api/run");
    await page.route("**/api/run", (route) => route.fulfill({ json: { kind: "run", ok: false, phase: "run", compileOutput: "", stdout: "", stderr: "", exitCode: 139, error: "Segmentation fault", durationMs: 10 } }));
    await page.getByRole("button", { name: /^Run/ }).click();
    await expect(output(page).getByRole("heading", { name: /Runtime error/ })).toBeVisible();
  });

  test("the output panel stays out of aria-live", async ({ page }) => {
    await page.goto(lesson);
    await reject(page);
    await page.route("**/api/run", (route) => route.abort("failed"));
    await page.getByRole("button", { name: /^Test/ }).click();
    await expect(output(page).getByRole("alert")).toBeVisible();
    await expect(page.locator("#panel-output")).not.toHaveAttribute("aria-live");
  });
});

test("line wrap persists, resets, and never overflows a phone", async ({ page }) => {
  await page.setViewportSize({ width: 375, height: 812 });
  await page.addInitScript(() => localStorage.setItem("bm:workspace:tour-done", "1")); // the first-visit tour popover would sit over the settings menu
  await page.goto(lesson);
  await reject(page);
  await page.getByRole("tab", { name: "Code & feedback" }).click();
  const editor = page.getByRole("textbox", { name: /code editor/ });
  await editor.click();
  await page.keyboard.press("ControlOrMeta+a");
  await page.keyboard.insertText(`// ${"long ".repeat(60)}\n`);
  const lines = page.locator(".cm-content");
  const overflow = () => page.evaluate(() => document.documentElement.scrollWidth <= innerWidth);
  const tall = async () => (await page.locator(".cm-line").first().boundingBox())!.height;
  const before = await tall();
  await page.getByRole("button", { name: /Settings & shortcuts/ }).click();
  await page.getByRole("checkbox", { name: "Wrap long lines" }).check();
  await page.keyboard.press("Escape");
  await expect.poll(tall).toBeGreaterThan(before * 2);
  expect(await overflow()).toBe(true);
  expect(await page.evaluate(() => localStorage.getItem("bm:editor:line-wrap"))).toBe("1");
  await expect(lines).toBeVisible();
  await page.reload();
  await page.getByRole("tab", { name: "Code & feedback" }).click();
  await page.getByRole("button", { name: /Settings & shortcuts/ }).click();
  await expect(page.getByRole("checkbox", { name: "Wrap long lines" })).toBeChecked();
  await page.getByRole("button", { name: "Default view" }).click();
  await expect(page.getByRole("checkbox", { name: "Wrap long lines" })).not.toBeChecked();
  expect(await page.evaluate(() => localStorage.getItem("bm:editor:line-wrap"))).toBeNull();
});
