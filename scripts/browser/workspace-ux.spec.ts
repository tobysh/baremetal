import { test, expect, type Page } from "@playwright/test";

// Anonymous editor sessions: no database fixtures are needed beyond the scratch server from npm run test:regressions.
const lesson = "/learn/first-steps/hello-world";
const edit = async (page: Page, code: string) => {
  const editor = page.getByRole("textbox", { name: /code editor/ });
  await editor.click();
  await page.keyboard.press("ControlOrMeta+a");
  await page.keyboard.insertText(code);
};
const reject = (page: Page) => page.getByRole("button", { name: "Reject analytics" }).click().catch(() => {});

test("a running program can be cancelled", async ({ page }) => {
  await page.goto(lesson);
  await reject(page);
  await edit(page, "int main(void) { for (;;) {} }\n");
  await page.getByRole("button", { name: /^Run/ }).click();
  await expect(page.getByText("Compiling and running…")).toBeVisible();
  await page.getByRole("button", { name: "Cancel" }).click();
  await expect(page.getByText("Cancelled.", { exact: true }).first()).toBeVisible();
  await expect(page.getByRole("button", { name: /^Run/ })).toBeEnabled();
  await expect(page.getByRole("button", { name: "Cancel" })).toHaveCount(0);
});

test("Reset can be undone from the toast with the keyboard", async ({ page }) => {
  await page.goto(lesson);
  await reject(page);
  await edit(page, "// my own code\nint main(void) { return 0; }\n");
  const editor = page.getByRole("textbox", { name: /code editor/ });
  await page.getByRole("button", { name: "Reset" }).click();
  await expect(editor).not.toContainText("my own code");
  await page.keyboard.press("Tab");
  await expect(page.getByRole("button", { name: "Undo" })).toBeFocused();
  await page.keyboard.press("Enter");
  await expect(editor).toContainText("my own code");
});

test("failing tests come first and passing ones fold away", async ({ page }) => {
  await page.goto(lesson);
  await reject(page);
  await page.route("**/api/run", (route) => route.fulfill({ json: { kind: "test", compiled: true, compileOutput: "", results: [
    { name: "alpha passes", passed: true }, { name: "beta fails", passed: false, message: "nope" }, { name: "gamma passes", passed: true },
  ], allPassed: false, output: "", error: null, saved: false, reward: null } }));
  await page.getByRole("button", { name: /^Test/ }).click();
  const panel = page.getByRole("tabpanel", { name: "Output" });
  await expect(panel.getByText("beta fails")).toBeVisible();
  await expect(panel.getByText("alpha passes")).not.toBeVisible();
  await panel.getByText("2 passed ✓").click();
  await expect(panel.getByText("alpha passes")).toBeVisible();
  await expect(page.getByRole("status").filter({ hasText: "2 of 3 tests passing" })).toHaveCount(1);
});

test("the output panel is not itself a live region", async ({ page }) => {
  await page.goto(lesson);
  await reject(page);
  expect(await page.locator("#panel-output [aria-live], #panel-tutor [aria-live]").count()).toBe(0);
});

test("the reading pane divider works from the keyboard on wide screens", async ({ page }) => {
  await page.setViewportSize({ width: 1280, height: 900 });
  await page.goto(lesson);
  await reject(page);
  const divider = page.getByRole("separator", { name: "Resize the reading pane" });
  await divider.focus();
  const before = Number(await divider.getAttribute("aria-valuenow"));
  await page.keyboard.press("ArrowRight");
  await expect(divider).toHaveAttribute("aria-valuenow", String(before + 2));
  await page.keyboard.press("Home");
  await expect(divider).toHaveAttribute("aria-valuenow", "32");
});

test("a collapsible task summary appears above the editor on phones only", async ({ page }) => {
  await page.setViewportSize({ width: 375, height: 812 });
  await page.goto(lesson);
  await reject(page);
  await page.getByRole("tab", { name: "Code & feedback" }).click();
  await expect(page.getByText("Task", { exact: true })).toBeVisible();
  await page.setViewportSize({ width: 1280, height: 900 });
  await expect(page.getByText("Task", { exact: true })).not.toBeVisible();
});

test("course filters are plain links that hide non-matching courses", async ({ page }) => {
  await page.goto("/learn");
  const filters = page.getByRole("region", { name: "Filter courses" });
  await filters.getByRole("link", { name: "Rust", exact: true }).click();
  await expect(page).toHaveURL(/\/learn\?lang=rust/);
  await expect(page.locator('[data-track="rust-foundations"]')).toBeVisible();
  await expect(page.locator('[data-track="c-memory"]')).toBeHidden();
  await filters.getByRole("link", { name: "Advanced", exact: true }).click();
  await expect(page).toHaveURL(/lang=rust&level=Advanced/);
  await filters.getByRole("link", { name: "All" }).first().click();
  await expect(page).toHaveURL(/\/learn\?level=Advanced/);
});
