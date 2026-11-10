import { test, expect, type Page } from "@playwright/test";
import { MongoClient, ObjectId } from "mongodb";
import { createHash, randomBytes } from "node:crypto";
import { getTracks } from "../../src/lib/curriculum";
import { SECURE_COOKIES, SESSION_COOKIE } from "../../src/lib/session-cookie";

const base = process.env.TEST_BASE_URL ?? "http://127.0.0.1:3100";
const dbName = process.env.MONGODB_DB;
if (!dbName?.startsWith("bm_regression_")) throw new Error("Dashboard tests require the scratch database created by npm run test:regressions.");
const mongo = new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017");
const db = mongo.db(dbName);
const userId = new ObjectId();
const token = randomBytes(32).toString("hex");
const tracks = getTracks();
const zero = tracks.find((track) => track.slug === "first-steps")!;
const memory = tracks.find((track) => track.slug === "c-memory")!;
const go = tracks.find((track) => track.slug === "go-foundations")!;
const login = (page: Page) => page.context().addCookies([{ name: SESSION_COOKIE, value: token, domain: new URL(base).hostname, path: "/", httpOnly: true, secure: SECURE_COOKIES, sameSite: "Strict" }]);

test.beforeAll(async () => {
  await mongo.connect();
  await db.collection("users").insertOne({ _id: userId, name: "Dashboard fixture", email: `dashboard-${userId}@example.test`, username: `dash-${randomBytes(4).toString("hex")}`, profilePublic: false, passwordHash: "unused", emailVerified: true, experience: "none", timezone: "UTC", role: "user", xp: 0, sessionVersion: 0, createdAt: new Date("2026-01-01") });
  await db.collection("sessions").insertOne({ userId, tokenHash: createHash("sha256").update(token).digest("hex"), sessionVersion: 0, createdAt: new Date(), expiresAt: new Date(Date.now() + 3_600_000) });
});
test.afterAll(async () => { await mongo.close(); });
test.beforeEach(async ({ page }) => {
  await db.collection("progress").deleteMany({ userId });
  await db.collection("users").updateOne({ _id: userId }, { $set: { xp: 0, experience: "none" } });
  await login(page);
});

test("level progress starts at zero at each boundary and fills within that level", async ({ page }) => {
  const cases = [
    { xp: 0, label: "Level 1 progress: 0 of 8 XP", width: "0%" },
    { xp: 7, label: "Level 1 progress: 7 of 8 XP", width: "88%" },
    { xp: 8, label: "Level 2 progress: 0 of 16 XP", width: "0%" },
    { xp: 16, label: "Level 2 progress: 8 of 16 XP", width: "50%" },
    { xp: 24, label: "Level 3 progress: 0 of 36 XP", width: "0%" },
    { xp: 42, label: "Level 3 progress: 18 of 36 XP", width: "50%" },
    { xp: 1000, label: "1000 XP, top level", width: "100%" },
  ];
  for (const { xp, label, width } of cases) {
    await db.collection("users").updateOne({ _id: userId }, { $set: { xp } });
    await page.goto("/dashboard");
    const meter = page.getByRole("img", { name: label, exact: true });
    await expect(meter).toBeVisible();
    expect(await meter.locator(":scope > div").evaluate((element) => (element as HTMLElement).style.width)).toBe(width);
  }
});

test("starting recommendations explain novice and experienced routes without mobile overflow", async ({ page }) => {
  for (const { experience, track, reason, width } of [
    { experience: "none", track: zero, reason: "You said you have never written code, so this starts from zero.", width: 320 },
    { experience: "experienced", track: memory, reason: "You said you are an experienced developer, so you can start straight at pointers and memory.", width: 1440 },
  ]) {
    await db.collection("users").updateOne({ _id: userId }, { $set: { experience } });
    await page.setViewportSize({ width, height: 900 });
    await page.goto("/dashboard");
    const resume = page.getByRole("region", { name: "Resume course" });
    await expect(resume.getByRole("heading", { level: 1 })).toHaveText(track.lessons[0].title);
    await expect(resume.getByText(reason, { exact: true })).toBeVisible();
    await expect(resume.getByRole("link", { name: /^Start first lesson/ })).toHaveAttribute("href", `/learn/${track.slug}/${track.lessons[0].slug}`);
    expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
  }
});

test("returning learners resume unfinished work before experience-based starting routes", async ({ page }) => {
  const lesson = go.lessons[2];
  await db.collection("users").updateOne({ _id: userId }, { $set: { experience: "experienced" } });
  await db.collection("progress").insertOne({ userId, exerciseId: lesson.exercise.id, trackSlug: go.slug, status: "started", code: "saved work", attempts: 0, startedAt: new Date(), updatedAt: new Date() });
  await page.goto("/dashboard");
  const resume = page.getByRole("region", { name: "Resume course" });
  await expect(resume.getByRole("heading", { level: 1 })).toHaveText(lesson.title);
  await expect(resume.getByText("You left this one unfinished. Pick up where you stopped.", { exact: true })).toBeVisible();
  await expect(resume.getByRole("link", { name: /^Continue/ })).toHaveAttribute("href", `/learn/${go.slug}/${lesson.slug}`);
});
