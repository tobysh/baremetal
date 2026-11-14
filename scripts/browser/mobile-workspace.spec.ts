import { test, expect, type Page } from "@playwright/test";

const lesson = "/learn/c-memory/pointers-swap";
const reject = (page: Page) => page.getByRole("button", { name: "Reject analytics" }).click().catch(() => {});
test.use({ viewport: { width: 375, height: 600 }, hasTouch: true });

test("mobile reading and coding keep their own scroll positions and editor draft", async ({ page }) => {
  await page.goto(lesson);
  await reject(page);
  const read = page.getByRole("tab", { name: "Read lesson", exact: true });
  const code = page.getByRole("tab", { name: "Code & feedback", exact: true });
  await page.evaluate(() => window.scrollTo({ top: 650, behavior: "instant" }));
  const readingScroll = await page.evaluate(() => window.scrollY);
  expect(readingScroll).toBeGreaterThan(300);
  await expect(code).toBeInViewport();
  await code.click();
  const editor = page.getByRole("textbox", { name: /code editor/ });
  await expect(editor).toBeVisible();
  await editor.click();
  await page.keyboard.press("ControlOrMeta+a");
  await page.keyboard.insertText("// retained mobile draft\nint main(void) { return 0; }\n");
  await page.evaluate(() => window.scrollTo({ top: 200, behavior: "instant" }));
  const codingScroll = await page.evaluate(() => window.scrollY);
  expect(codingScroll).toBeGreaterThan(50);
  await read.click();
  await expect.poll(() => page.evaluate(() => window.scrollY)).toBeCloseTo(readingScroll, 0);
  await code.click();
  await expect.poll(() => page.evaluate(() => window.scrollY)).toBeCloseTo(codingScroll, 0);
  await expect(editor).toContainText("retained mobile draft");
  await expect(page.getByRole("button", { name: /^Run/ })).toBeEnabled();
  await expect(page.getByRole("button", { name: /^Test/ })).toBeEnabled();
});

test("selection, vertical scrolling and cancelled touches never become swipe navigation", async ({ page }) => {
  await page.goto(lesson);
  await reject(page);
  const read = page.getByRole("tab", { name: "Read lesson", exact: true });
  const gesture = async (kind: "selection" | "vertical" | "cancel" | "swipe") => page.evaluate((kind) => {
    const target = document.querySelector<HTMLElement>("#view-panel-lesson article p")!;
    const selection = window.getSelection()!;
    selection.removeAllRanges();
    if (kind === "selection") {
      const range = document.createRange();
      range.selectNodeContents(target);
      selection.addRange(range);
    }
    const touch = (x: number, y: number) => new Touch({ identifier: 1, target, clientX: x, clientY: y });
    const send = (type: string, x: number, y: number) => {
      const point = touch(x, y);
      target.dispatchEvent(new TouchEvent(type, { bubbles: true, touches: type === "touchend" || type === "touchcancel" ? [] : [point], changedTouches: [point] }));
    };
    send("touchstart", 280, 200);
    if (kind === "vertical") send("touchmove", 278, 250);
    if (kind === "cancel") send("touchcancel", 270, 200);
    send("touchend", 90, 205);
  }, kind);

  for (const kind of ["selection", "vertical", "cancel"] as const) {
    await gesture(kind);
    await expect(read).toHaveAttribute("aria-selected", "true");
  }
  await gesture("swipe");
  await expect(page.getByRole("tab", { name: "Code & feedback", exact: true })).toHaveAttribute("aria-selected", "true");
});

for (const width of [320, 375, 390, 768, 1024, 1440]) {
  test(`lesson views do not overflow the page at ${width}px`, async ({ page }) => {
    await page.setViewportSize({ width, height: 812 });
    await page.goto(lesson);
    await reject(page);
    await expect(page.locator("#view-panel-lesson")).toBeVisible();
    const overflow = () => page.evaluate(() => document.documentElement.scrollWidth - document.documentElement.clientWidth);
    expect(await overflow(), `reading view at ${width}px`).toBeLessThanOrEqual(1);
    if (width < 1024) await page.getByRole("tab", { name: "Code & feedback", exact: true }).click();
    await expect(page.getByRole("textbox", { name: /code editor/ })).toBeVisible();
    expect(await overflow(), `coding view at ${width}px`).toBeLessThanOrEqual(1);
  });
}

test("Run and Test remain grouped and reachable in a short mobile viewport", async ({ page }) => {
  // Reduced viewport height approximates lost keyboard space; it does not emulate a real software keyboard.
  await page.setViewportSize({ width: 375, height: 320 });
  await page.goto(lesson);
  await reject(page);
  await page.getByRole("tab", { name: "Code & feedback", exact: true }).click();
  await expect(page.getByRole("textbox", { name: /code editor/ })).toBeVisible();
  const tour = page.getByRole("region", { name: "Quick tour", exact: true });
  if (await tour.isVisible()) await tour.getByRole("button", { name: "Skip", exact: true }).click();
  const run = page.getByRole("button", { name: /^Run/ });
  const check = page.getByRole("button", { name: /^Test/ });
  await run.scrollIntoViewIfNeeded();
  await expect(run).toBeInViewport({ ratio: 1 });
  await run.click({ trial: true });
  await check.click({ trial: true });
  await expect(page.getByRole("group", { name: "Run code and tests", exact: true })).toBeInViewport({ ratio: 1 });
  const runBounds = (await run.boundingBox())!;
  const testBounds = (await check.boundingBox())!;
  expect(Math.abs(runBounds.y - testBounds.y)).toBeLessThan(2);
  expect(testBounds.x - (runBounds.x + runBounds.width)).toBeGreaterThanOrEqual(0);
  expect(testBounds.x - (runBounds.x + runBounds.width)).toBeLessThanOrEqual(24);
});

test("Back to task respects reduced motion and leaves its heading below the sticky tabs", async ({ page }) => {
  await page.emulateMedia({ reducedMotion: "reduce" });
  await page.addInitScript(() => {
    const scroll = Element.prototype.scrollIntoView;
    Element.prototype.scrollIntoView = function (options?: boolean | ScrollIntoViewOptions) {
      if (this.id === "task" && typeof options === "object") (window as typeof window & { taskJumpMotion?: ScrollBehavior }).taskJumpMotion = options.behavior;
      return scroll.call(this, options);
    };
  });
  await page.goto(lesson);
  await reject(page);
  await page.getByRole("tab", { name: "Code & feedback", exact: true }).click();
  await page.keyboard.press("Alt+1");
  await expect(page.getByRole("tab", { name: "Read lesson", exact: true })).toHaveAttribute("aria-selected", "true");
  await expect(page.locator("#view-panel-lesson")).toBeFocused();
  await expect.poll(() => page.evaluate(() => (window as typeof window & { taskJumpMotion?: ScrollBehavior }).taskJumpMotion)).toBe("instant");
  const tabs = (await page.getByRole("tablist", { name: "Lesson views" }).boundingBox())!;
  await expect.poll(async () => (await page.locator("#task").boundingBox())!.y).toBeGreaterThanOrEqual(tabs.y + tabs.height);
});
