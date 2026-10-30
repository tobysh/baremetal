import { defineConfig } from "@playwright/test";

export default defineConfig({
  testDir: "./scripts/browser",
  workers: 1,
  timeout: 45_000,
  expect: { timeout: 10_000 },
  retries: 0,
  reporter: "list",
  use: {
    baseURL: process.env.TEST_BASE_URL ?? "http://127.0.0.1:3100",
    browserName: "chromium",
    trace: "retain-on-failure",
    launchOptions: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE ? { executablePath: process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE } : {},
  },
});
